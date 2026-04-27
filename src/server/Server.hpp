#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <vector>
#include <map>
#include <sys/epoll.h>  //this is for epoll system calls and data structures
#include <sys/socket.h> //this is for socket system calls and data structures
#include <netinet/in.h> //this is for sockaddr_in structure and related constants
#include <arpa/inet.h>  //this is for inet_pton function to convert IP addresses
#include <fcntl.h>      //this is for fcntl system call to set non-blocking mode
#include <unistd.h>     //this is for close system call
#include <fstream>      //this is for file operations
#include <sstream>      //this is for stringstream to build error messages
#include <exception>
#include <algorithm>
#include "../http/Request.hpp"
#include<set>
#include <cstring> // for memset

extern bool g_shutdown ; // Global flag to signal shutdown across the server, allowing for graceful termination of the main loop and cleanup of resources when a shutdown signal is received (e.g., SIGINT or SIGTERM)

#include "../config/Config.hpp"
#include "Client.hpp"

class Server
{
private:
    Config config;

    std::vector<int> listen_fds;
    int epoll_fd;

    std::map<int, Client *> clients;
    std::map<int, std::vector<ServerBlock*> > fd_to_servers;

public:
    Server(Config &config);
     ~Server();
    void setupSockets();
    void initEpoll();
    void start();

    void handleEvent(struct epoll_event &event);

    void acceptClient(int listen_fd);
    void handleClient(int client_fd, uint32_t events);
    void handleRead(int client_fd);
    void processRequest(int client_fd);
    void handleWrite(int client_fd);


    bool isListenSocket(int fd);
    void closeClient(int fd);
};
#endif