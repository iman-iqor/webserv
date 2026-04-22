#include "Client.hpp"
Client::Client(int fd)
{
    this->fd = fd;
    request_complete = false;
    ready_to_send = false;
}

Client::~Client() {
    // ✅ Clean up if needed
    buffer.clear();
    response.clear();
}