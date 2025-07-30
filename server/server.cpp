#include "./server.h"

int get_listener_socket(){
    int listener;	 // Listening socket descriptor
	int yes=1;		// For setsockopt() SO_REUSEADDR, below
	int rv;

	addrinfo hints;
	addrinfo *ai, *p;

	// Get us a socket and bind it
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
		fprintf(stderr, "pollserver: %s\n", gai_strerror(rv));
		exit(1);
	}
	
	for(p = ai; p != NULL; p = p->ai_next) {
		listener = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol);
		if (listener < 0) { 
			continue;
		}
		
		// Lose the pesky "address already in use" error message
		setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int));

		if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
			close(listener);
			continue;
		}

		break;
	}

	// If we got here, it means we didn't get bound
	if (p == NULL) {
		return -1;
	}

	freeaddrinfo(ai); // All done with this

	// Listen
	if (listen(listener, 10) == -1) {
		return -1;
	}

	return listener;
}

void process_connections(int listener, int exitfd, int *fd_count, int *fd_size,
		pollfd **pfds, char*** usernames, pthread_mutex_t* mutex){
    for(int i = 0; i < *fd_count; i++) {
		// Check if someone's ready to read
		if ((*pfds)[i].revents & (POLLIN | POLLHUP)) {
			// We got one!!
			if ((*pfds)[i].fd == listener) {
				// If we're the listener, it's a new connection
				pthread_mutex_lock(mutex);
				handle_new_connection(listener, fd_count, fd_size, pfds, usernames);
				pthread_mutex_unlock(mutex);
			} else {
				// Otherwise we're just a regular client
				pthread_mutex_lock(mutex);
				handle_client_data(listener, exitfd,fd_count, *pfds, *usernames, &i);
				pthread_mutex_unlock(mutex);
			}
		}
	}
}

void handle_new_connection(int listener, int *fd_count,
		int *fd_size,  pollfd** pfds, char*** usernames)
{
	sockaddr_storage remoteaddr; // Client address
	socklen_t addrlen;
	int newfd;  // Newly accept()ed socket descriptor
	char remoteIP[INET6_ADDRSTRLEN];

	addrlen = sizeof remoteaddr;
	newfd = accept(listener, ( sockaddr *)&remoteaddr,
			&addrlen);

	if (newfd == -1) {
		perror("accept");
	} else {
		add_to_pfds(pfds, usernames,newfd, fd_count, fd_size);
		printf("pollserver: new connection from %s on socket %d\n",
				inet_ntop2(&remoteaddr, remoteIP, sizeof remoteIP),
				newfd);
	}
}

void add_to_pfds(pollfd **pfds, char*** usernames,int newfd, int *fd_count,
		int *fd_size){
	//add recv here for username
    if(*fd_count == *fd_size){
		std::cout << "inside realloc\n";
        *fd_size *= 2; 
        *pfds = (pollfd*) realloc(*pfds, sizeof(**pfds) * (*fd_size));
		
		char** tmp = (char**) realloc(*usernames, sizeof(char*) * (*fd_size));
		if (tmp != NULL) {
			*usernames = tmp;
		} else {
			std::cout << "this is not working\n";
		}
    }
	//adding fd
    (*pfds)[*fd_count].fd = newfd;
    (*pfds)[*fd_count].events = POLLIN;
    (*pfds)[*fd_count].revents = 0;

	//username
	char buf[250];
	memset(buf,0,sizeof(buf));
	int nbytes = recv(newfd, buf, sizeof buf, 0);
	(*usernames)[*fd_count] = (char*) malloc(strlen(buf) + 1);
	
	if ((*usernames)[*fd_count]) {
		strcpy((*usernames)[*fd_count], buf);
	}
	(*fd_count)++;
}

const char *inet_ntop2(void *addr, char *buf, size_t size)
{
	sockaddr_storage *sas = (sockaddr_storage *) addr;
	sockaddr_in *sa4;
	void *src;

	switch (sas->ss_family) {
		case AF_INET:
			sa4 = (sockaddr_in *) addr;
			src = &(sa4->sin_addr);
			break;
		default:
			return NULL;
	}

	return inet_ntop(sas->ss_family, src, buf, size);
}

void handle_client_data(int listener, int exitfd, int *fd_count,
		 pollfd *pfds, char** usernames, int *pfd_i)
{
	char buf[256];	// Buffer for client data
	memset(buf,0, sizeof(buf));
	int nbytes = recv(pfds[*pfd_i].fd, buf, sizeof buf, 0);

	int sender_fd = pfds[*pfd_i].fd;
	
	if (nbytes <= 0) { // Got error or connection closed by client
		if (nbytes == 0) {
			// Connection closed
			printf("pollserver: socket %d hung up\n", sender_fd);
		} else {
			perror("recv");
		}

		close(pfds[*pfd_i].fd); // Bye!

		del_from_pfds(pfds, usernames,*pfd_i, fd_count);

		// reexamine the slot we just deleted
		(*pfd_i)--;

	} else { // We got some good data from a client
		printf("pollserver: recv from fd %d: %.*s \n", sender_fd,
				nbytes, buf);
		// Send to everyone!
		std::cout << "sending to everyone!\n";
		char combined[250];
		memset(combined, 0, sizeof(combined));
		char semi_colon[3] = ": ";
		strcat(combined,usernames[*pfd_i]);
		strcat(combined,semi_colon);
		strcat(combined, buf);
		int length = sizeof(combined);
		for(int j = 0; j < *fd_count; j++) {
			int dest_fd = pfds[j].fd;
			// Except the listener and ourselves
			if (dest_fd != listener && dest_fd != sender_fd && dest_fd != exitfd) {
				if (send(dest_fd, combined, length, 0) == -1) {
					perror("send");
				}
			}
		}
	}
}

void del_from_pfds( pollfd pfds[], char** usernames, int i, int *fd_count)
{
	// Copy the one from the end over this one
	pfds[i] = pfds[*fd_count-1];
	//free should be added here(triple pointer should be added then)
	usernames[i] = usernames[*fd_count-1];

	(*fd_count)--;
}

void* poller(void* args)
{
	server_data* sdata = (server_data*) args; 
    while(1){
		int poll_count = poll(sdata->fds, *(sdata->fd_count), -1);
		
        if (poll_count == -1) {
			perror("poll");
			exit(1);
		}
		if(server_running == 0){
			break;
		}
        process_connections(*(sdata->listener), *(sdata->exit_fd),(sdata->fd_count), (sdata->fd_size), &(sdata->fds), sdata->usernames ,sdata->mutex);
    }
    //close fds & usernames
	for(int i{}; i < *(sdata->fd_count); ++i){
		close(sdata->fds[i].fd);
		free(sdata->usernames[0][i]);
	}
	free(sdata->fds);
	free(*(sdata->usernames));
	return nullptr;
}

void* broadcast_to_clients(void* args){
	broadcast_data* clients = (broadcast_data*) args;
	while(1){
		int len, bytes_sent;
		std::string msg_cpp;
		getline(std::cin, msg_cpp);
		if(msg_cpp == "/exit"){
			server_running = 0;
			u_int64_t val = 1;//eventfd can accept <=8bytes
			if(write(clients->exit_fd, &val, sizeof(val))==-1) perror("write");
			std::cout << "wrote to exit_fd\n";
			break;
		}
		const char *msg = msg_cpp.c_str();
		len = strlen(msg);
		if(len <= 0){
			perror("send");
		}else{
			pthread_mutex_lock(clients->mutex);
			std::cout << '\n';
			std::cout << "sending to everyone\n";
			char combined[250];
			memset(combined, 0, sizeof(combined));
			strcat(combined,"SERVER: ");
			strcat(combined, msg);
			int length = sizeof(combined);
			for(int j = 0; j < *(clients->fd_count); j++) {
				int dest_fd = clients->fds[j].fd;
				// Except the listener
				if (dest_fd != *(clients->listener) && dest_fd != clients->exit_fd){
					if (send(dest_fd, combined, length, 0) == -1) {
						perror("send");
					}
				}
			}
			pthread_mutex_unlock(clients->mutex);
		}
	}
	return nullptr;

}

void start(){
	int listener;
    //storing connections in a vector
	int fd_size = 5;
	int fd_count = 0;
	pollfd *pfds = (pollfd*) malloc(sizeof *pfds * fd_size);
    //storing usernames in a vector
    char** usernames = (char**) malloc(sizeof(char*)*fd_size);
    for(int i{}; i < fd_size; ++i){
        usernames[i] = NULL;
    }
    //listening socket
    listener = get_listener_socket();

    if(listener == -1){
        fprintf(stderr, "error getting listening socket\n");
        exit(1);
    }

    //reaping dead processes
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if(sigaction(SIGCHLD, &sa, nullptr) == -1){
        perror("sigaction");
        exit(1);
    }


    //accept an incoming connection
    char servername[] = "SERVER: "; 
    usernames[0] = (char*) malloc(strlen(servername)+1);
    if (usernames[fd_count]) {
		strcpy(usernames[fd_count], servername);
	}
    pfds[0].fd = listener;
    pfds[0].events = POLLIN;

    fd_count = 1; // For the listener

    int exit_event = eventfd(0,0);

    char exit_name[] = "EXIT_EVENT";
    usernames[1] = (char*) malloc(strlen(servername)+1);
    if (usernames[fd_count]) {
		strcpy(usernames[fd_count], exit_name);
	}
    pfds[1].fd = exit_event;
    pfds[1].events = POLLIN;

    fd_count = 2;

    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, nullptr);

    //polling data
    server_data s_data;
    s_data.exit_fd = &exit_event;
    s_data.fd_size = &fd_size;
    s_data.fd_count = &fd_count;
    s_data.fds = pfds;
    s_data.listener = &listener;
    s_data.mutex = &mutex;
    s_data.usernames = &usernames;

    //broadcasting data
    broadcast_data b_clients;
    b_clients.exit_fd = exit_event;
    b_clients.fd_count = &fd_count;
    b_clients.fds = pfds;
    b_clients.listener = &listener;
    b_clients.mutex = &mutex;


    //threading starts here
    pthread_t* thread_handles;
    thread_handles = (pthread_t*) malloc(thread_count*sizeof(pthread_t));
    puts("pollserver: waiting for connections...");

    pthread_create(&thread_handles[0], nullptr,
            poller, &s_data);
    pthread_create(&thread_handles[1], nullptr,
            broadcast_to_clients, &b_clients);

    pthread_join(thread_handles[0], nullptr);
    pthread_join(thread_handles[1], nullptr);
    
    
    std::cout << "massive day for the unemployed\n";
}

