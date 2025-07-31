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

#define PORT "3490"
#define BACKLOG 10

extern int server_running;

extern int thread_count;

int get_listener_socket();

void process_connections(int listener, int exitfd ,int *fd_count, int *fd_size,
		pollfd **pfds, char*** usernames ,pthread_mutex_t* mutex);

void handle_new_connection(int listener, int *fd_count, int *fd_size, 
        pollfd** pfds, char*** usernames);

void add_to_pfds(pollfd **pfds, char*** usernames, int newfd, int *fd_count,
		int *fd_size);

const char *inet_ntop2(void *addr, char *buf, size_t size);

void handle_client_data(int listener, int exitfd,int *fd_count,
		pollfd *pfds, char*** usernames, int *pfd_i);

void del_from_pfds( pollfd pfds[], char*** usernames, int i, int *fd_count);

void* broadcast_to_clients(void* args);

void* poller(void* args);

void start();

struct server_data{
	pollfd* fds;
	char*** usernames;
	int* fd_count;
	int* listener;
	int* exit_fd;
	int* fd_size;
	pthread_mutex_t* mutex;
};

struct broadcast_data{
	pollfd* fds;
	int* exit_fd;
	int* fd_count;
	int* listener;
	pthread_mutex_t* mutex;
};


#endif