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
#include <set>
#include <cstring> 
#include "../http/Request.hpp"
#include "../http/Response.hpp"
#include "../Router/Router.hpp"
#include "../config/Config.hpp"
#include "Client.hpp"

extern volatile sig_atomic_t g_shutdown;


enum FDType
{
    SERVER,
    CLIENT,
    CGI_PIPE
};

struct CgiContext {
    int     client_fd;   // which client is waiting
    int     pipe_fd;     // read-end of stdout pipe
    pid_t   child_pid;   // for waitpid / timeout
    std::string output;  // accumulates CGI stdout
};
struct EpollData
{
    int fd;
    FDType type;
    Client *client;
    CgiContext *cgi;    // non-null when type == CGI_PIPE
};

class Server
{
private:
    Config config;
    std::vector<int> listen_fds;
    int epoll_fd;
    std::map<int, Client *> clients;
    std::map<int, std::vector<ServerBlock *> > fd_to_servers;
    std::map<int, EpollData *> epoll_data;
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
    void handleRead(Client *client);
    void handleWrite(Client *client);
    void processRequest(int client_fd);
    void handleClientError(Client *client, const HttpException &e);


    //route
    RouteInfo FileUploadRoute(const RouteInfo &route, Request &request);
    RouteInfo DeleteFile(const RouteInfo &route);

    //CGI
    void handleCGI(EpollData *data, uint32_t events);
    void launchCGI(int client_fd, RouteInfo &route, Client *client);
    void setupCGIEnv(Request &req,RouteInfo &route);
    
    void closeClient(int fd);
    std::string intToString(size_t n);
};
#endif