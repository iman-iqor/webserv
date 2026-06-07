#include "Client.hpp"

Client::Client(int listen_fd, int client_fd)
{
    this->fd = client_fd;
    this->listen_fd = listen_fd;
    ready_to_send = false;
    request_complete = false;
    expected_body_size = 0;
    
    // Initialize CGI state
    cgi_state = NULL;
}

Client::~Client() {
    
    
    //Clean up buffer and response
    buffer.clear();
    response.clear();
}