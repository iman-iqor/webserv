#include "Client.hpp"
Client::Client(int listen_fd, int client_fd)
{
    this->fd = client_fd;
    this->listen_fd = listen_fd;
    ready_to_send = false;
    
    listen_fd = -1;
    
}

Client::~Client() {
    // ✅ Clean up if needed
    buffer.clear();
    response.clear();
}