#include "CgiHandler.hpp"


char **CgiHandler::build_envp(const std::map< std::string, std::string > &env_map) {
    char **envp = new char*[env_map.size() + 1];
    int i = 0;
    std::map<std::string, std::string>::const_iterator it = env_map.begin();
    for (; it != env_map.end(); it++) {
        std::string env_entry = it->first + "=" + it->second;
        envp[i] = new char[env_entry.size() + 1];
        std::strcpy(envp[i], env_entry.c_str());
        i++;
    }
    envp[i] = NULL;
    return envp;
}

void CgiHandler::free_envp(char **envp) {
    for (int i = 0; envp[i] != NULL; i++) {
        delete[] envp[i];
    }
    delete[] envp;
}

void CgiHandler::set_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        throw std::runtime_error("Failed to get file flags");
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw std::runtime_error("Failed to set non-blocking mode");
    }
}

bool open_pipes(int fdi[2], int fdo[2])
{
    if (pipe(fdi) == -1 || pipe(fdo) == -1) {
        
        throw std::runtime_error("pipe error");
    }
}

void CgiHandler::start(
    Client *client,
    const std::string &cgi_path,
    std::string &bin_path,
    int epoll_fd,
    const std::map< std::string, std::string > &env_map
) {
    int req_pipe[2] = {-1, -1};
    int res_pipe[2] = {-1, -1};


}