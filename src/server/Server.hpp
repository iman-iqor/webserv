#ifndef SERVER_HPP
#define SERVER_HPP

#include<iostream>
#include<vector>
#include<map>
#include<sys/epoll.h>//this is for epoll system calls and data structures 
#include<sys/socket.h>//this is for socket system calls and data structures
#include<netinet/in.h>//this is for sockaddr_in structure and related constants
#include<arpa/inet.h>//this is for inet_pton function to convert IP addresses
#include<fcntl.h>//this is for fcntl system call to set non-blocking mode
#include<unistd.h>//this is for close system call
#include<fstream>//this is for file operations
#include<sstream>//this is for stringstream to build error messages
#include<exception>
#include"../config/Config.hpp"
#include"../http/Request.hpp"
#include"../http/Request.hpp"



#include"config/Config.hpp"

class Server
{
    private:
        Config config; //get this from oumaima
        std::vector<int> listen_fds;//this will store the file descriptors for the listening sockets
        int epoll_fd; //epoll file descriptor
        std::map<int,std::string> client_buffers; //this will store the partial request data for each client socket
        
    public:
        Server(Config &config);
        void run();


};
#endif