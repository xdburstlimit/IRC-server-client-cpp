#include "./client.h"


addrinfo* get_address(const char* ip){
    int status;
    addrinfo hints;
    addrinfo *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;//ipv4
    hints.ai_socktype = SOCK_STREAM;//tcp stream sockets

    if((status = getaddrinfo(ip, PORT, &hints, &res))!=0){
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    return res;

}

int get_server_socket(){
    addrinfo *server_info, *p;
    int yes=1;
    int socket_fd;
    const char* ip = "127.0.0.1";
    server_info = get_address(ip);
    char s[INET_ADDRSTRLEN];

    for(p = server_info; p != nullptr; p = p->ai_next){
        //make socket
        if ((socket_fd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }
        
        //bind socket to port
        if (connect(socket_fd, p->ai_addr, p->ai_addrlen) == -1){
            close(socket_fd);
            perror("client: connect");
            continue;
        }
        break;
    }
    freeaddrinfo(server_info);

    //some check for pointer == nullptr
    
    if(p == nullptr){
        fprintf(stderr,"client: failed to connect\n");
        exit(1);
    }
    //inet_ntop(AF_INET, get_in_addr((struct sockaddr*) p->ai_addr), s, sizeof(s)); //address converts binary to number and dots notation(?)
    return socket_fd;
}

void *get_in_addr(struct sockaddr *sa){
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void* send_data(void* args){
    send_args* my_args = (send_args*) args;
    while(1){
        int len, bytes_sent;
        std::string msg_cpp;
        getline(std::cin, msg_cpp);
        if(msg_cpp == "/exit"){
            client_running = 0;
            u_int64_t val = 1;//eventfd can accept <=8bytes
			if(write(my_args->exit_fd, &val, sizeof(val))==-1) perror("write");
            break;
        }
        if(server_running == 0) break;     
        const char *msg = msg_cpp.c_str();
        len = strlen(msg);
        if(len <= 0){
            perror("send");
        }else{
            if (send(my_args->socket_fd, msg, len, 0) == -1) {
                perror("send");
            }
        }
    }

    return nullptr;
}

void* recv_data(void *args){
    recv_args* my_args = (recv_args*) args;
    while(1){
        int poll_count = poll(my_args->pfds, my_args->fd_count,-1);
        if(client_running == 0) break;
        //will have to get client's own fd to check if they set
        if(my_args->pfds[0].revents & POLLIN){
            char buf[512];
            memset(buf, 0, sizeof(buf));
            int byte_count = recv(my_args->socket_fd, buf, sizeof(buf), 0);
            if(byte_count<=0){
                if(byte_count == 0){
                    printf("server closed, you can safely close the application now\n");
                    server_running = 0;
                    break;
                }
            }else{
                std::cout << buf << '\n';
            }
        }
    }
    return nullptr;
}

void start(){
    pthread_t* thread_handles;
    thread_handles = (pthread_t*)  malloc(thread_count*sizeof(pthread_t));
    send_args args_send_thread;
    recv_args args_recv_thread;

    //getting username from client
    int len;
    char username_i[50];
    std::cout << "Please insert a username:\n";
    fgets(username_i, sizeof(username_i),stdin);
    size_t eos = sizeof(username_i);
    for(int i{}; i < eos; ++i){ 
        if(username_i[i] == '\n'){ 
            username_i[i] = '\0';
            break;
        }
    }
    const char* username = username_i;
    std::string cmp = username;
    if(cmp == "/exit"){
        return;
    }else{
        //sending username to server
        int server_socket = get_server_socket();
        len = strlen(username);
        if(len <= 0){
            perror("send");
        }else{
            if (send(server_socket, username, len, 0) == -1) {
                perror("username");
            }
        }

        //Filling in args_recv_thread
        args_recv_thread.socket_fd = server_socket;
        args_recv_thread.pfds[0].fd = server_socket;
        args_recv_thread.pfds[0].events = POLLIN;

        args_recv_thread.fd_count = 1;

        int exit_event = eventfd(0,0);

        args_recv_thread.pfds[1].fd = exit_event;
        args_recv_thread.pfds[1].events = POLLIN;

        args_recv_thread.fd_count = 2;

        //Filling in args_send_thread
        args_send_thread.socket_fd = server_socket;
        args_send_thread.exit_fd = exit_event;
        
        pthread_create(&thread_handles[0], nullptr,
                recv_data, (void*) &args_recv_thread);
        pthread_create(&thread_handles[1], nullptr,
            send_data, (void*) &args_send_thread);

        
        for(long thread = 0; thread <thread_count; ++thread){//joining threads
            pthread_join(thread_handles[thread], nullptr);
        }
        std::cout << "shalom\n";
        
        return;
    }
}


