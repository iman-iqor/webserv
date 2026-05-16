#include "Client.hpp"
#include <signal.h>
#include <sys/wait.h>

void Client::cleanup_cgi(void)
{
    if (cgi_state)
    {
        if (cgi_state->stdin_fd >= 0)
            close(cgi_state->stdin_fd);
        if (cgi_state->stdout_fd >= 0)
            close(cgi_state->stdout_fd);
        if (cgi_state->pid > 0)
        {
            kill(cgi_state->pid, SIGTERM);
            waitpid(cgi_state->pid, NULL, 0);
        }
        delete cgi_state;
        cgi_state = NULL;
    }
}

Client::Client(int listen_fd, int client_fd, std::vector< ServerBlock* > *sv_block)
{
    this->fd = client_fd;
    this->listen_fd = listen_fd;
    request.sv_blocks = sv_block;
    ready_to_send = false;
    cgi_state = NULL;
    
    listen_fd = -1;
}

Client::~Client() {
    cleanup_cgi();
    buffer.clear();
    response.clear();
}