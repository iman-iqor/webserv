#include "CgiHandler.hpp"

#include "../server/Server.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <vector>
#include <cstring>
#include <stdexcept>

char **CgiHandler::build_envp(const std::map<std::string, std::string> &env)
{
    char **envp = new char *[env.size() + 1];
    std::map<std::string, std::string>::const_iterator it = env.begin();
    size_t index = 0;

    try
    {
        for (; it != env.end(); ++it, ++index)
        {
            const std::string entry = it->first + "=" + it->second;
            envp[index] = new char[entry.size() + 1];
            std::strcpy(envp[index], entry.c_str());
        }
    }
    catch (...)
    {
        CgiHandler::free_envp(envp);
        throw;
    }
    envp[index] = NULL;
    return envp;
}

void CgiHandler::free_envp(char **envp)
{
    if (!envp)
        return;
    for (size_t i = 0; envp[i] != NULL; ++i)
        delete [] envp[i];
    delete [] envp;
}

void CgiHandler::set_non_blocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        throw std::runtime_error("fcntl O_NONBLOCK failed");
}

void CgiHandler::close_fd_pair(int pipefd[2])
{
    if (pipefd[0] >= 0)
        close(pipefd[0]);
    if (pipefd[1] >= 0)
        close(pipefd[1]);
    pipefd[0] = -1;
    pipefd[1] = -1;
}

static void close_inherited_fds(void)
{
    long max_fd = sysconf(_SC_OPEN_MAX);
    if (max_fd < 0)
        max_fd = 1024;
    for (long fd = 3; fd < max_fd; ++fd)
        close(fd);
}

CgiProcessResult CgiHandler::start(Client *client,
                                   const std::string &cgi_path,
                                   const std::string &bin_path,
                                   int epoll_fd,
                                   const std::map<std::string, std::string> &env)
{
    int request_pipe[2] = {-1, -1};
    int response_pipe[2] = {-1, -1};
    if (pipe(request_pipe) == -1 || pipe(response_pipe) == -1)
    {
        close_fd_pair(request_pipe);
        close_fd_pair(response_pipe);
        throw std::runtime_error("pipe failed");
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        close_fd_pair(request_pipe);
        close_fd_pair(response_pipe);
        throw std::runtime_error("fork failed");
    }

    if (pid == 0)
    {
        char **envp = NULL;
        std::vector<std::string> args;
        std::vector<char *> argv;

        if (dup2(request_pipe[0], STDIN_FILENO) == -1
            || dup2(response_pipe[1], STDOUT_FILENO) == -1)
            _exit(1);

        close(request_pipe[0]);
        close(request_pipe[1]);
        close(response_pipe[0]);
        close(response_pipe[1]);
        close_inherited_fds();

        try
        {
            args.push_back(bin_path);
            args.push_back(cgi_path);
            argv.push_back(const_cast<char *>(args[0].c_str()));
            argv.push_back(const_cast<char *>(args[1].c_str()));
            argv.push_back(NULL);
            envp = build_envp(env);
        }
        catch (...)
        {
            _exit(1);
        }

        execve(bin_path.c_str(), &argv[0], envp);
        free_envp(envp);
        _exit(1);
    }

    close(request_pipe[0]);
    close(response_pipe[1]);
    set_non_blocking(request_pipe[1]);
    set_non_blocking(response_pipe[0]);

    EpollData *stdout_data = new EpollData;
    stdout_data->type = CGI_PIPE;
    stdout_data->fd = response_pipe[0];
    stdout_data->client = client;

    struct epoll_event stdout_event;
    std::memset(&stdout_event, 0, sizeof(stdout_event));
    stdout_event.data.ptr = stdout_data;
    stdout_event.events = EPOLLIN | EPOLLHUP;  // Include EPOLLHUP to detect child close
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, response_pipe[0], &stdout_event) == -1)
    {
        delete stdout_data;
        close_fd_pair(request_pipe);
        close_fd_pair(response_pipe);
        throw std::runtime_error("epoll_ctl add CGI stdout failed");
    }

    EpollData *stdin_data = new EpollData;
    stdin_data->type = CGI_PIPE;
    stdin_data->fd = request_pipe[1];
    stdin_data->client = client;

    struct epoll_event stdin_event;
    std::memset(&stdin_event, 0, sizeof(stdin_event));
    stdin_event.data.ptr = stdin_data;
    stdin_event.events = EPOLLOUT;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, request_pipe[1], &stdin_event) == -1)
    {
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, response_pipe[0], NULL);
        delete stdout_data;
        delete stdin_data;
        close_fd_pair(request_pipe);
        close_fd_pair(response_pipe);
        throw std::runtime_error("epoll_ctl add CGI stdin failed");
    }

    CgiProcessResult result;
    result.pid = pid;
    result.request_write_fd = request_pipe[1];
    result.response_read_fd = response_pipe[0];
    return result;
}

CgiProcessResult cgi_start(Client *client,
                           const std::string &cgi_path,
                           const std::string &bin_path,
                           int epoll_fd)
{
    std::map<std::string, std::string> env;

    env["GATEWAY_INTERFACE"] = "CGI/1.1";
    env["SCRIPT_FILENAME"] = cgi_path;
    env["SCRIPT_NAME"] = cgi_path;
    env["REDIRECT_STATUS"] = "200";
    env["REQUEST_METHOD"] = client->request.get_method();
    env["REQUEST_URI"] = client->request.get_path();

    return CgiHandler::start(client, cgi_path, bin_path, epoll_fd, env);
}