#ifndef CLIENT_HPP
#define CLIENT_HPP
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

#include "../config/Config.hpp"

class Client
{
public:
    int fd; // file descriptor for the client socket

    std::string buffer;   // incoming request
    std::string response; // outgoing response
    int expected_body_size; // expected size of the request body, used for tracking how much of the body has been received so far

    int listen_fd;// file descriptor for the listening socket that accepted this client
    Request request;
    bool request_complete;
    bool ready_to_send;

    Client(int listen_fd, int client_fd, std::vector< ServerBlock* > *sv_block);
    ~Client();
};
#endif // CLIENT_HPP