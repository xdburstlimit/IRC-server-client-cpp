#ifndef _CLIENT_
#define _CLIENT_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/eventfd.h>
#include <poll.h>
#include <iostream>
#include <pthread.h>

#include <vector>

#define PORT "3490"

int get_server_socket();

extern int server_running;

extern int client_running;

extern int msg_i;

extern std::string display_messages;

extern std::string users_connected;

void *get_in_addr(struct sockaddr *sa);

void send_data(char msg[],int socket_fd);

void* recv_data(void *info);

void start();

struct recv_args{
    int socket_fd;
    pollfd pfds[2];
    int fd_count;
    std::vector <std::string>* chat_history;
};

struct send_args{
    int socket_fd;
    int exit_fd;
};

#endif