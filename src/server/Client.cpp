#include "Client.hpp"
Client::Client(int listen_fd, int client_fd, std::vector< ServerBlock* > *sv_block)
{
    this->fd = client_fd;
    this->listen_fd = listen_fd;
    request.sv_blocks = sv_block;
    ready_to_send = false;
    
    listen_fd = -1;
    // client_ip = "";
    // client_port = -1;
}

Client::~Client() {
    // ✅ Clean up if needed
    buffer.clear();
    response.clear();
}