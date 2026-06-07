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
#include <signal.h>
#include "../http/Request.hpp"
#include "../http/Response.hpp"

#include "../Router/Router.hpp"
#include <set>
#include <cstring> // for memset

extern volatile sig_atomic_t g_shutdown;

#include "../config/Config.hpp"
#include "Client.hpp"

enum FDType
{
    SERVER,
    CLIENT,
    CGI_PIPE
};

struct EpollData
{
    int fd;
    FDType type;
    Client *client;
};

class Server
{
private:
    Config config;

    std::vector<int> listen_fds;
    int epoll_fd;

    std::map<int, Client *> clients;
    std::map<int, std::vector<ServerBlock *> > fd_to_servers;
    Router *router;

public:
    Server(Config &config);
    ~Server();
    void setupSockets();
    void initEpoll();
    void start();

    void handleEvent(struct epoll_event &event);

    void acceptClient(int listen_fd);
    void handleClient(EpollData *data, uint32_t events);
    void handleCGI(EpollData *data, uint32_t events);
    void handleRead(Client *client);
    void handleWrite(Client *client);
    void processRequest(int client_fd);
    void handleFileUpload(int client_fd, const RouteInfo &route, const Request &request);
    void handleDeleteFile(int client_fd, const RouteInfo &route);
    std::string buildErrorResponse(int code, const std::string &message);
    void handleClientError(Client *client, const HttpException &e);

    void closeClient(int fd);
    void switchToWrite(int client_fd);
    std::string intToString(size_t n);
};
#endif