#include "Server.hpp"
#include <signal.h>
#include <sys/wait.h>
#include "../http/Exceptions.hpp"

void Server::setupCGIEnv(Request &req,RouteInfo &route)
{
    setenv("REQUEST_METHOD", req.get_method_str().c_str(), 1);
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
    if(req.get_method_str() == "POST")
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




// This function will fork a child process to execute the CGI script
// and set up pipes for communication between the server and the CGI process.
// The implementation details are complex and involve setting up environment variables,
// handling input/output redirection, and managing the CGI process lifecycle.
void Server::launchCGI(Client *client, RouteInfo route)
{
	const std::string &body = client->request.get_body();
    bool has_body = 0;
    if(client->request.get_method_str() == "POST" && !body.empty())
        has_body = 1;       

    int stdout_pipe[2];
    int stdin_pipe[2]={-1,-1};

    if(pipe(stdout_pipe) == -1)
    {
        throw InternalServerErrorException("Failed to create stdout pipe");
    }

    if(has_body && pipe(stdin_pipe) == -1)
    {
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        throw InternalServerErrorException("Failed to create stdin pipe");
    }

    pid_t pid = fork();
    if(pid == -1)
    {
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        if(has_body)
        {
            close(stdin_pipe[0]);
            close(stdin_pipe[1]);
        }
        throw InternalServerErrorException("Fork failed");
    }

    if(pid == 0)
    {
        close(stdout_pipe[0]);
        if(dup2(stdout_pipe[1],STDOUT_FILENO == -1))
            _exit(1);
        close(stdout_pipe[1]);

        if(has_body)
        {
            close(stdin_pipe[1]);
            if(dup2(stdin_pipe[0],STDIN_FILENO) == -1)
                _exit(1);
            close(stdin_pipe[0]);
        }
        //close all other open fds  the child inherited(epoll_fd,client_fds...)
        //simple approach :clos all fds above stderr
        for(int fd = 3; fd < 1024; ++fd)
            close(fd);
        
        setupCGIEnv(client->request,route);

        char* argv[] =
        {
            const_cast<char *>(route.cgi_path.c_str()),
            const_cast<char *>(route.cgi_string.c_str()),
            NULL
        };
        execve(route.cgi_path.c_str(),argv,environ);
        // If execve returns, it means it failed
        _exit(1);
    }
    close(stdout_pipe[1]);

    if(has_body)
    {
        
    }
}