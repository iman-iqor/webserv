#include"Server.hpp"

void Server::acceptClient(int listen_fd)
{
    int client_fd = accept(listen_fd, NULL, NULL); 
    
    if (client_fd < 0) {
        
        return;
    }

    clients[client_fd] = new Client(listen_fd, client_fd);

    struct epoll_event event;
    event.events = EPOLLIN;
    EpollData* data = new EpollData;
    data->fd = client_fd;
    data->type = CLIENT;
    data->client = clients[client_fd];
    event.data.ptr = data;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) 
    {
        std::cerr << "Failed to add client to epoll" << std::endl;
        close(client_fd); 
        delete data;
        throw std::runtime_error("epoll_ctl client add failed");
    }
   
    std::cout << "New client connected: " << client_fd << std::endl;
}