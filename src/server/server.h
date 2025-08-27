#ifndef _SERVER_
#define _SERVER_

#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <sys/wait.h>
#include <sys/eventfd.h>
#include <signal.h>
#include <pthread.h>


#include "imgui.h"
#include "imgui-SFML.h"
#include "SFML/Graphics.hpp"

#include <vector>

#define PORT "3490"
#define BACKLOG 10

struct server_data{
	pollfd** fds;
	char*** usernames;
	int* fd_count;
	int* listener;
	int* exit_fd;
	int* fd_size;
	std::vector <std::string>* chat_history;
	pthread_mutex_t* mutex;
};

struct broadcast_data{
	int* exit_fd;
	int* fd_count;
	int* listener;
	std::vector <std::string>* chat_history;
	pthread_mutex_t* mutex;
};

extern int server_running;

extern int thread_count;

extern int msg_i;
extern std::string display_messages;

extern std::string users_connected;

extern int fd_start;

const int username_length = 32;
const int message_length = 256;
const int total_length = username_length + message_length;


int get_listener_socket();

void process_connections(int listener, int exitfd ,int *fd_count, int *fd_size,
		pollfd **pfds, char*** usernames ,pthread_mutex_t* mutex, std::vector <std::string>* chat_history);

void handle_new_connection(int listener, int *fd_count, int *fd_size, 
        pollfd** pfds, char*** usernames);

void add_to_pfds(pollfd **pfds, char*** usernames, int newfd, int *fd_count,
		int *fd_size);

void handle_client_data(int listener, int exitfd,int *fd_count,
		pollfd **pfds, char*** usernames, int *pfd_i, std::vector <std::string>* chat_history);

void del_from_pfds( pollfd** pfds, char*** usernames, int i, int *fd_count);

void broadcast_to_clients(broadcast_data* clients, pollfd* pfds,char* msg);

void* poller(void* args);

void send_user_list(std::string display_users, pollfd** pfds, int fd_count);

void chat_window();

void user_window();

void start();



#endif