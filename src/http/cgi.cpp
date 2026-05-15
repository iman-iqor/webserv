
#include <Client.hpp>
#include <Server.hpp>
#include <unistd.h>

void save_pipe(int pipe_fd[2][2], int epoll_fd) {
    EpollData* data = new EpollData;
    data->type = CGI_PIPE;
    data->fd = pipe_fd[0][0];
    data->client = NULL; // No client associated with this pipe
    struct epoll_event event = {.data.ptr = data, .events = EPOLLIN};
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, data->fd, &event) == -1) {
        std::cerr << "Failed to add CGI pipe to epoll" << std::endl;
        close(pipe_fd[0][0]);
        close(pipe_fd[0][1]);
        close(pipe_fd[1][0]);
        close(pipe_fd[1][1]);
        delete data;
        throw std::runtime_error("epoll_ctl CGI pipe add failed");
    }

    EpollData* data2 = new EpollData;
    data2->type = CGI_PIPE;
    data2->fd = pipe_fd[1][0];
    data2->client = NULL; // No client associated with this pipe
    struct epoll_event event2 = {.data.ptr = data2, .events = EPOLLOUT};
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, data2->fd, &event2) == -1) {
        std::cerr << "Failed to add CGI pipe to epoll" << std::endl;
        close(pipe_fd[0][0]);
        close(pipe_fd[0][1]);
        close(pipe_fd[1][0]);
        close(pipe_fd[1][1]);
        delete data;
        delete data2;
        throw std::runtime_error("epoll_ctl CGI pipe add failed");
    }
}

int cgi_start(Client *client, std::string &cgi_path, std::string &bin_path, int epoll_fd)
{
    int pipe_fd[2][2];  // Pipe for parent to child (write to child)
    if (pipe(pipe_fd[0]) == -1 || pipe(pipe_fd[1]) == -1) {
        return -1;
    }
    save_pipe(pipe_fd, epoll_fd); // Save the pipe file descriptors for later use in the CGI process;
    pid_t pid = fork();
    if (pid < 0) {
        return -1;
    } else if (pid == 0) {

        // Child process
    } else {
        // Parent process
    }
    return 0;
}