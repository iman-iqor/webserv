#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include <map>
#include <string>

#include "../server/Client.hpp"

struct CgiProcessResult
{
    pid_t pid;
    int request_write_fd;
    int response_read_fd;
};

class CgiHandler
{
public:
    static char **build_envp(const std::map<std::string, std::string> &env);
    static void free_envp(char **envp);
    static void set_non_blocking(int fd);
    static void close_fd_pair(int pipefd[2]);
    static CgiProcessResult start(Client *client,
                                  const std::string &cgi_path,
                                  const std::string &bin_path,
                                  int epoll_fd,
                                  const std::map<std::string, std::string> &env);
};

CgiProcessResult cgi_start(Client *client,
                           const std::string &cgi_path,
                           const std::string &bin_path,
                           int epoll_fd);

#endif
