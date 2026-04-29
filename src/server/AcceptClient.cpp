#include"Server.hpp"

void Server::acceptClient(int listen_fd)
{
    int client_fd = accept(listen_fd, NULL, NULL); // Accept a new incoming connection on the listening socket, which returns a new file descriptor for the client socket. The client's address information is not needed in this case, so NULL is passed for the address and its length.
    
    if (client_fd < 0) {
        
        return;
    }


    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = client_fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) 
    {
        std::cerr << "Failed to add client to epoll" << std::endl;
        close(client_fd);  // ✅ Close if can't add to epoll
        throw std::runtime_error("epoll_ctl client add failed");
    }

    clients[client_fd] = new Client(client_fd);// Create a new Client object for the accepted client connection and store it in the clients map using the client file descriptor as the key, allowing the server to manage and track the state of each connected client separately.
    clients[client_fd]->listen_fd = listen_fd; // Store the listening socket file descriptor that accepted this client in the Client object for later reference, which can be useful for determining which server block to use when processing requests from this client based on the listening socket it connected to.
   
    std::cout << "New client connected: " << client_fd << std::endl;
}