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

#include "imgui.h"
#include "imgui-SFML.h"

#include "SFML/Graphics.hpp"


#define PORT "3490"

int get_server_socket();

extern int server_running;

extern int client_running;

extern std::string display_messages;

extern std::string users_connected;

const int buf_size = 1024;
const int username_length = 32;
const int message_length = 256;
const int total_length = username_length + message_length;

void send_data(char msg[],int socket_fd);

void* recv_data(void *info);

void chat_window();

void user_window();

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