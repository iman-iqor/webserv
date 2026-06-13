#include"Server.hpp"

void Server::acceptClient(int listen_fd)
{



    int client_fd = accept(listen_fd, NULL, NULL); 
    
    if (client_fd < 0) {
        
        return;
    }

    Client *client = new Client(listen_fd, client_fd);
    clients[client_fd] = client;

    struct epoll_event event;
    event.events = EPOLLIN;
    EpollData* data = new EpollData;
    data->fd = client_fd;
    data->type = CLIENT;
    data->client = clients[client_fd];
    epoll_data[client_fd] = data;
    event.data.ptr = data;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) 
    {
        std::cerr << "Failed to add client to epoll" << std::endl;
        close(client_fd); 
        delete data;
        epoll_data.erase(client_fd);
        throw std::runtime_error("epoll_ctl client add failed");
    }
   

    ServerBlock *server_block = NULL;
    if (fd_to_servers.find(client->listen_fd) != fd_to_servers.end())
    {
        if (fd_to_servers[client->listen_fd].size() > 0)
        {
            server_block = fd_to_servers[client->listen_fd][0];
        }
    }

    client->request.setServerBlock(server_block);

    std::cout << "New client connected: " << client_fd << std::endl;
}