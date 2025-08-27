#include "./server.h"

int get_listener_socket(){ 
    int listener;	 
	int yes=1;		
	int rv;

	addrinfo hints;
	addrinfo *ai, *p;

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
		
	
		setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int));

		if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
			close(listener);
			continue;
		}

		break;
	}

	if (p == NULL) {
		return -1;
	}

	freeaddrinfo(ai); 

	if (listen(listener, 10) == -1) {
		return -1;
	}

	return listener;
}

void process_connections(int listener, int exitfd, int *fd_count, int *fd_size,
		pollfd **pfds, char*** usernames, pthread_mutex_t* mutex, std::vector <std::string>* chat_history){ 
    for(int i = 0; i < *fd_count; i++) {
		if ((*pfds)[i].revents & (POLLIN | POLLHUP)) {
			if ((*pfds)[i].fd == listener) {// if a new user connected
				pthread_mutex_lock(mutex);
				handle_new_connection(listener, fd_count, fd_size, pfds, usernames);
				pthread_mutex_unlock(mutex);
			} else {//actual data or if a user disconnected
				pthread_mutex_lock(mutex);
				handle_client_data(listener, exitfd,fd_count, pfds, usernames, &i, chat_history);
				pthread_mutex_unlock(mutex);
			}
		}
	}
}

void handle_new_connection(int listener, int *fd_count,
		int *fd_size,  pollfd** pfds, char*** usernames) {
	sockaddr_storage remoteaddr; 
	socklen_t addrlen;
	int newfd;  
	char remoteIP[INET_ADDRSTRLEN];

	addrlen = sizeof remoteaddr;
	newfd = accept(listener, ( sockaddr *)&remoteaddr,
			&addrlen);

	if (newfd == -1) {
		perror("accept");
	} else {
		add_to_pfds(pfds, usernames,newfd, fd_count, fd_size);
	}
}

void add_to_pfds(pollfd **pfds, char*** usernames,int newfd, int *fd_count,
		int *fd_size){
    if(*fd_count == *fd_size){
        *fd_size *= 2;

        pollfd* tmp_pfds = (pollfd*) realloc(*pfds, sizeof(**pfds) * (*fd_size));
		if(tmp_pfds != NULL){
			*pfds = tmp_pfds;
		}else{
			std::cerr << "pfds realloc failed\n";
		}
		
		char** tmp_usernames = (char**) realloc(*usernames, sizeof(char*) * (*fd_size));
		if (tmp_usernames != NULL) {
			*usernames = tmp_usernames;
		} else {
			std::cerr << "usernames realloc failed\n";
		}
    }
	//adding fd
    (*pfds)[*fd_count].fd = newfd;
    (*pfds)[*fd_count].events = POLLIN;
    (*pfds)[*fd_count].revents = 0;

	//username
	char buf[username_length];
	memset(buf,0,sizeof(buf));
	int nbytes = recv(newfd, buf, sizeof buf, 0);
	if(nbytes <= 0){
		if(nbytes == 0){
			printf("client disconnected\n");
		}else{
			perror("username: ");
		}
	}else{
		(*usernames)[*fd_count] = (char*) malloc(strlen(buf) + 1);
		if ((*usernames)[*fd_count]) {
			strcpy((*usernames)[*fd_count], buf);
		}
		std::string new_user((*usernames)[*fd_count]);
		users_connected += (new_user + '\n');
		++(*fd_count);
		send_user_list(users_connected,pfds,*fd_count);
	}
}


void handle_client_data(int listener, int exitfd, int *fd_count,
		 pollfd **pfds, char*** usernames, int *pfd_i, std::vector <std::string>* chat_history){
	char buf[message_length];	// Buffer for client data
	memset(buf,0, sizeof(buf));
	int nbytes = recv((*pfds)[*pfd_i].fd, buf, sizeof buf, 0);

	int sender_fd = (*pfds)[*pfd_i].fd;
	
	if (nbytes <= 0) { 
		if (nbytes == 0) {
			printf("pollserver: socket %d hung up\n", sender_fd);
		} else {
			perror("recv");
		}

		close((*pfds)[*pfd_i].fd); 

		del_from_pfds(pfds, usernames,*pfd_i, fd_count);

		--(*pfd_i);

	} else {
		printf("pollserver: recv from fd %d: %.*s \n", sender_fd,
				nbytes, buf);
		char combined[total_length];
		memset(combined, 0, sizeof(combined));
		char semi_colon[3] = ": ";
		strcat(combined,(*usernames)[*pfd_i]);
		strcat(combined,semi_colon);
		strcat(combined, buf);
		int length = sizeof(combined);
		std::string store_msg = combined;
		(*(chat_history)).push_back(store_msg);
		std::vector<std::string>& vecRef = *chat_history; 						
		std::string a = vecRef[msg_i];	
		display_messages += (a + '\n'); 
        ++msg_i;
		for(int j = 0; j < *fd_count; j++) {
			int dest_fd = (*pfds)[j].fd;
			if (dest_fd != listener && dest_fd != sender_fd && dest_fd != exitfd) {
				if (send(dest_fd, combined, length, 0) == -1) {
					perror("send");
				}
			}
		}
	}
}

void del_from_pfds( pollfd** pfds, char*** usernames, int i, int *fd_count){
	(*pfds)[i] = (*pfds)[*fd_count-1];
	free((*usernames)[i]);
	(*usernames)[i] = (*usernames)[*fd_count-1];

	--(*fd_count);
	users_connected = "";
	for(int i{fd_start}; i < (*fd_count); ++i){
		std::string new_user((*usernames)[i]);
		users_connected += (new_user + '\n');
	}
	send_user_list(users_connected,pfds,*fd_count);

}

void* poller(void* args){
	server_data* sdata = (server_data*) args; 
    while(1){
		int poll_count = poll(*(sdata->fds), *(sdata->fd_count), -1);
        if (poll_count == -1) {
			perror("poll");
			exit(1);
		}
		if(server_running == 0){
			break;
		}
        process_connections(*(sdata->listener), *(sdata->exit_fd),(sdata->fd_count), (sdata->fd_size), sdata->fds, sdata->usernames ,sdata->mutex,sdata->chat_history);
    }
    //close fds & usernames
	std::cout << *(sdata->fd_count) << '\n';
	for(int i{}; i < *(sdata->fd_count); ++i){
		close((*(sdata->fds))[i].fd);
		free((*(sdata->usernames))[i]);
	}
	
	free(*(sdata->fds));
	free(*(sdata->usernames));
	
	return nullptr;
}

void broadcast_to_clients(broadcast_data* clients,pollfd* pfds,char* msg){
	int len, bytes_sent;
	len = strlen(msg);
	if(len <= 0){
		perror("send");
	}else{
		pthread_mutex_lock(clients->mutex);
		char combined[256];
		memset(combined, 0, sizeof(combined));
		strcat(combined,"SERVER: ");
		strcat(combined, msg);
		std::string store_msg = combined;
		(*(clients->chat_history)).push_back(store_msg);
		int length = sizeof(combined);
		for(int j{fd_start}; j < *(clients->fd_count); j++) {
			int dest_fd = pfds[j].fd;
			if (dest_fd != *(clients->listener) && dest_fd != *(clients->exit_fd)){
				if (send(dest_fd, combined, length, 0) == -1) {
					perror("send");
				}
			}
		}
		pthread_mutex_unlock(clients->mutex);
	}
	return;

}

void start(){
	sf::RenderWindow w_imgui(sf::VideoMode(745, 715), "Server",sf::Style::Titlebar|sf::Style::Close);
	ImGui::SFML::Init(w_imgui);
	sf::Clock deltaClock;
	int listener;

    float size_pixels = 18;
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    ImFont* font = io.Fonts->AddFontFromFileTTF("../../../res/server/font.ttf", size_pixels, NULL, io.Fonts->GetGlyphRangesDefault());
    io.FontDefault = font;
    ImGui::SFML::UpdateFontTexture();

    //storing connections in a vector
	int fd_size = 10;
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
	++fd_start;
    int exit_event = eventfd(0,0);

    char exit_name[] = "EXIT_EVENT";
    usernames[1] = (char*) malloc(strlen(servername)+1);
    if (usernames[fd_count]) {
		strcpy(usernames[fd_count], exit_name);
	}
    pfds[1].fd = exit_event;
    pfds[1].events = POLLIN;

    fd_count = 2;
	++fd_start;

    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, nullptr);

    std::vector<std::string> chat_history;


    //polling data
    server_data s_data;
    s_data.exit_fd = &exit_event;
    s_data.fd_size = &fd_size;
    s_data.fd_count = &fd_count;
    s_data.fds = &pfds;
    s_data.listener = &listener;
    s_data.mutex = &mutex;
    s_data.usernames = &usernames;
    s_data.chat_history = &chat_history;

    //broadcasting data
    broadcast_data b_clients;
    b_clients.exit_fd = &exit_event;
    b_clients.fd_count = &fd_count;
    b_clients.listener = &listener;
    b_clients.mutex = &mutex;
    b_clients.chat_history = &chat_history;


    //threading starts here
    pthread_t* thread_handles;
    thread_handles = (pthread_t*) malloc(sizeof(pthread_t));
    pthread_create(thread_handles, nullptr,
            poller, &s_data);


    char msg[message_length] = "";
	int msg_size = IM_ARRAYSIZE(msg);

	while(w_imgui.isOpen()){
		sf::Event event;
		while(w_imgui.pollEvent(event)){
			ImGui::SFML::ProcessEvent(event);
			if(event.type == sf::Event::Closed){
				server_running = 0;
				u_int64_t val = 1;//eventfd can accept <=8bytes
				if(write(*(b_clients.exit_fd), &val, sizeof(val))==-1) perror("write");
                w_imgui.close();
				break;
			}
		}
		//ImGui windows start here
		ImGui::SFML::Update(w_imgui, deltaClock.restart());
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0xee, 0xe8, 0xd6, 0xff));

		chat_window();

		user_window();
	
        ImGui::SetNextWindowPos(ImVec2(0,675));
		ImGui::SetNextWindowSize(ImVec2(880,0));
		ImGui::Begin(" ",NULL,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize);
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
        ImGui::SetWindowFocus(" ");
        ImGui::InputText("", msg, msg_size);
        ImGui::SameLine();
        if((ImGui::IsKeyPressed(ImGui::GetIO().KeyMap[ImGuiKey_Enter], false) &&ImGui::IsWindowFocused() || 
            ImGui::Button("Send", ImVec2(150,0))) && (strlen(msg) > 0 && strlen(msg) < message_length)){
            broadcast_to_clients(&b_clients, pfds,msg);
            memset(msg,0 ,msg_size);
            display_messages += (chat_history[msg_i] + '\n');
            ++msg_i;
        }
        ImGui::PopFont();
		ImGui::End();
		
		
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();



		w_imgui.clear();
		ImGui::SFML::Render(w_imgui);
        w_imgui.display();
	}
    pthread_join(*thread_handles, nullptr);
}

void send_user_list(std::string display_users, pollfd** pfds, int fd_count){
	//should maybe send fd_count through first. so the client recv knows how much space is required
	int user_list_size = fd_count * username_length;
	char combined[user_list_size];
	memset(combined,0,sizeof(combined));
	strcat(combined,"/");
	strcat(combined,display_users.c_str());
	int n = strlen(combined);
	for(int j = fd_start; j < fd_count; j++) {
		int dest_fd = (*pfds)[j].fd;
		if (send(dest_fd, combined, n, 0) == -1) {
			perror("send");
		}	
	}
}

void chat_window(){
	ImGui::SetNextWindowPos(ImVec2(0,0));
	ImGui::Begin("chat lobby",NULL,ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoTitleBar);
	ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
	ImGui::InputTextMultiline(" ",(char*) display_messages.c_str(),sizeof(display_messages),ImVec2(575,675),ImGuiInputTextFlags_ReadOnly);
	ImGui::PopFont();
	ImGui::End();
}

void user_window(){
	ImGui::SetNextWindowPos(ImVec2(580,0));
	ImGui::Begin("users", NULL ,ImGuiWindowFlags_NoTitleBar);
	ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
	ImGui::InputTextMultiline(" ",(char*) users_connected.c_str(),sizeof(users_connected),ImVec2(150,675),ImGuiInputTextFlags_ReadOnly);
	ImGui::PopFont();
	ImGui::End();
}
