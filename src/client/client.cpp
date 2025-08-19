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

void send_data(char msg[],int socket_fd){
    //send_args* my_args = (send_args*) args;
    int len{};
    len = strlen(msg);
    if(len <= 0){
        perror("send");
    }else{
        if (send(socket_fd, msg, len, 0) == -1) {
            perror("send");
        }
    }
    return;
}

void* recv_data(void *args){
    recv_args* my_args = (recv_args*) args;
    while(1){
        int poll_count = poll(my_args->pfds, my_args->fd_count,-1);
        if(client_running == 0) break;
        //will have to get client's own fd to check if they set
        if(my_args->pfds[0].revents & POLLIN){
            char buf[256];
            memset(buf, 0, sizeof(buf));
            int byte_count = recv(my_args->socket_fd, buf, sizeof(buf), 0);
            if(byte_count<=0){
                if(byte_count == 0){
                    std::string shut_down = "WARNING: Server closed, you can safely close the application now\n";
                    display_messages += shut_down;
                    server_running = 0;
                    break;
                }
            }else{
                if(buf[0] == '/'){
                    users_connected = "";
                    users_connected = buf+1;
                    memset(buf, 0, sizeof(buf));
                }else{
                    std::string buf_str = buf;
                    display_messages += buf_str + '\n';
                }
            }
        }
    }
    return nullptr;
}

void start(){
    return;
}


