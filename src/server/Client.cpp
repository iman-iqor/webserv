#include "Client.hpp"
Client::Client(int fd)
{
    this->fd = fd;
    request_complete = false;
    ready_to_send = false;
    request = Request();
    listen_fd = -1;
    client_ip = "";
    client_port = -1;
}

Client::~Client() {
    // ✅ Clean up if needed
    buffer.clear();
    response.clear();
}