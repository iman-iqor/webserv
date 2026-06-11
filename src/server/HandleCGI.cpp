#include "Server.hpp"
#include <signal.h>
#include <sys/wait.h>
#include "../http/Exceptions.hpp"

void Server::setupCGIEnv(Request &req,RouteInfo &route)
{
    setenv("REQUEST_METHOD", req.get_method().c_str(), 1);
    setenv("SCRIPT_FILENAME", route.cgi_string.c_str(), 1);
    setenv("PATH_INFO", req.get_path().c_str(), 1);
    setenv("QUERY_STRING",req.get_query_string().c_str(),1);
    setenv("GATEWAY_INTERFACE", "CGI/1.1", 1);
    setenv("SERVER_PROTOCOL", "HTTP/1.1", 1);
    setenv("SERVER_SOFTWARE", "Webserv/1.0", 1);
    setenv("REDIRECT_STATUS", "200", 1); // for PHP CGI

    const std::string &ct = req.getHeader("Content-Type");
    if (!ct.empty())
        setenv("CONTENT_TYPE", ct.c_str(), 1);
    
    //content lenght must always be set for post even if 0
    const std::string &body = req.get_body();
    if(req.get_method() == "POST")
    {
        std::ostringstream ss;
        ss << body.size();
        setenv("CONTENT_LENGTH", ss.str().c_str(), 1);
    }

    const std::string &cookies = req.getHeader("Cookie");
    if(!cookies.empty())
        setenv("HTTP_COOKIE", cookies.c_str(), 1);

    const std::string &host = req.getHeader("Host");

    if(!host.empty())
        setenv("HTTP_HOST", host.c_str(), 1);
}



void Server::launchCGI(int client_fd, RouteInfo &route, Client *client)
{
    int stdout_pipe[2];

    if (pipe(stdout_pipe) == -1)
        throw HttpException(500, "Internal Server Error", "pipe() failed");

    pid_t pid = fork();
    if (pid == -1)
    {
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        throw HttpException(500, "Internal Server Error", "fork() failed");
    }

    if (pid == 0)
    {
        close(stdout_pipe[0]);
        if (dup2(stdout_pipe[1], STDOUT_FILENO) == -1) _exit(1);
        close(stdout_pipe[1]);

        if (client->request.get_method() == "POST"
            && client->request.get_body_size() > 0)
        {
            int body_fd = open(client->request.get_filename().c_str(), O_RDONLY);
            if (body_fd == -1) _exit(1);
            if (dup2(body_fd, STDIN_FILENO) == -1) _exit(1);
            close(body_fd);
        }

        // close all inherited fds
        for (int fd = 3; fd < 1024; fd++)
            close(fd);

        setupCGIEnv(client->request, route);

        char *argv[] = {
            const_cast<char *>(route.cgi_path.c_str()),
            const_cast<char *>(route.cgi_string.c_str()),
            NULL
        };
        execve(route.cgi_path.c_str(), argv, environ);
        _exit(1);
    }

    close(stdout_pipe[1]);
    fcntl(stdout_pipe[0], F_SETFL, O_NONBLOCK);
}