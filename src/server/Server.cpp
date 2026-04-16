#include"Server.hpp"

Server::Server(Config &config)
{
    this->config = config;
    epoll_fd = -1; // Initialize epoll_fd to an invalid value


}
void Server::setupSockets()
{
    // Implementation for setting up listening sockets based on the configuration
    // This would involve creating sockets, binding them to the specified ports, and setting them to listen mode
    while()
}


void Server::start()
{
    // Implementation for starting the server
    // This would typically involve setting up the listening sockets, initializing epoll, and entering the main event loop
    
}