#include "Client.hpp"
#include "../http/CgiHandler.hpp"

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
    // Clean up CGI resources
    if (cgi_state && cgi_state->pid != -1)
    {
        kill(cgi_state->pid, SIGKILL);
        waitpid(cgi_state->pid, NULL, 0);
        close_pipes(cgi_state);
    }
    
    //Clean up buffer and response
    buffer.clear();
    response.clear();
}