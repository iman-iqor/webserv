#include"Server.hpp"

void Server::acceptClient(int listen_fd)
{
    int client_fd = accept(listen_fd, NULL, NULL); // Accept a new incoming connection on the listening socket, which returns a new file descriptor for the client socket. The client's address information is not needed in this case, so NULL is passed for the address and its length.
    
    if (client_fd < 0) {
        
        return;
    }

    clients[client_fd] = new Client(listen_fd, client_fd, &fd_to_servers[listen_fd]);// Create a new Client object for the accepted client connection and store it in the clients map using the client file descriptor as the key, allowing the server to manage and track the state of each connected client separately.

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
        close(client_fd);  // ✅ Close if can't add to epoll
        delete data;
        throw std::runtime_error("epoll_ctl client add failed");
    }
   
    std::cout << "New client connected: " << client_fd << std::endl;
}