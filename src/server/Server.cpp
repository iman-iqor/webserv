#include "Server.hpp"
#include <cstring> // for memset

Server::Server(Config &config)
{
    this->config = config;
    epoll_fd = -1; // Initialize epoll_fd to an invalid value
}

// Server.cpp
Server::~Server()
{
    // ✅ Close all client connections
    for (std::map<int, Client *>::iterator it = clients.begin();
         it != clients.end(); ++it)
    {
        int fd = it->first;
        delete it->second;
        close(fd);
    }
    clients.clear();

    // ✅ Remove all listen sockets from epoll
    for (size_t i = 0; i < listen_fds.size(); i++)
    {
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, listen_fds[i], NULL);
        close(listen_fds[i]);
    }
    listen_fds.clear();

    // ✅ Close epoll
    if (epoll_fd != -1)
    {
        close(epoll_fd);
    }

    std::cout << "Server shut down cleanly" << std::endl;
}

void Server::setupSockets()
{
    std::set<std::pair<std::string, int> > all;

    // 1. collect everything
    for (size_t i = 0; i < config.servers.size(); i++)
    {
        ServerBlock &server = config.servers[i];

        for (size_t j = 0; j < server.listen_directives.size(); j++)
        {
            all.insert(server.listen_directives[j]);
        }
    }

    // 2. create sockets from unique values
    for (std::set<std::pair<std::string, int> >::iterator it = all.begin();
         it != all.end(); ++it)
    {
        std::string ip = it->first;
        int port = it->second;

        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0)
            throw std::runtime_error("Failed to create socket");

        // find the port in listen directives and associate the socket with the corresponding server blocks
        for (size_t i = 0; i < config.servers.size(); i++)
        {
            ServerBlock &server = config.servers[i];

            for (size_t j = 0; j < server.listen_directives.size(); j++)
            {
                if (server.listen_directives[j].first == ip && server.listen_directives[j].second == port)
                {
                    fd_to_servers[sock].push_back(&server);
                }
            }
        }

        // allow this socket to reuse the address/port even if it’s still in TIME_WAIT
        int opt = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); // Allow reuse of the address

        fcntl(sock, F_SETFL, O_NONBLOCK); // Set the socket to non-blocking mode

        sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;   // IPv4
        addr.sin_port = htons(port); // Convert port to network byte order

        if (ip == "0.0.0.0")
            addr.sin_addr.s_addr = INADDR_ANY;
        else if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0)
            throw std::runtime_error("Invalid IP address");

        // binding means associating the socket with a specific IP address and port number on the local machine
        if (bind(sock, (sockaddr *)&addr, sizeof(addr)) < 0)
            throw std::runtime_error("Failed to bind socket to address");
        // Start listening for incoming connections on the socket with a backlog of SOMAXCONN (maximum allowed by the system)
        if (listen(sock, SOMAXCONN) < 0)
            throw std::runtime_error("Failed to listen on socket");
        // what turns a socket into a server socket that can accept connections
        listen_fds.push_back(sock);

        std::cout << "Listening on " << ip << ":" << port << std::endl;
    }
}
void Server::initEpoll()
{
    epoll_fd = epoll_create1(0); // Create an epoll instance and get a file descriptor for it to monitor events on multiple file descriptors efficiently
    if (epoll_fd == -1)
        throw std::runtime_error("epoll_create1 failed");

    struct epoll_event event;
    event.events = EPOLLIN;

    for (size_t i = 0; i < listen_fds.size(); i++)
    {
        event.data.fd = listen_fds[i]; // Store the file descriptor in the event data for later identification

        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fds[i], &event) == -1) // Register the listening socket with the epoll instance to monitor for incoming connections
            throw std::runtime_error("epoll_ctl ADD failed");
    }
}
void Server::start()
{
    struct epoll_event events[1024]; // Array to hold events returned by epoll_wait

    while (!g_shutdown) // Main server loop that continues running until a shutdown signal is received, allowing the server to handle incoming connections and client activity continuously
    {
        int nfds = epoll_wait(epoll_fd, events, 1024, -1); // Wait for events on the monitored file descriptors, blocking indefinitely until at least one event occurs. The events are stored in the events array, and nfds indicates how many events were returned.

        if (nfds < 0)
        {
            if (errno == EINTR)
                continue; // signal interrupted, just retry
            throw std::runtime_error("epoll_wait failed");
        }

        for (int i = 0; i < nfds; i++)
        {
            try
            {
                {
                    
                    handleEvent(events[i]); // Handle each event returned by epoll_wait, which could be a new incoming connection or activity on an existing client socket
                }
            }
            catch(const std::exception &e)
            {
                std::cerr << "Error handling event: " << e.what() << std::endl;
            }
            
        }
    }
}

void Server::handleEvent(struct epoll_event &event)
{
    int fd = event.data.fd;

    if (isListenSocket(fd)) // Check if the event is on a listening socket, which indicates a new incoming connection or activity on an existing client socket and if so, accept the new client connection
        acceptClient(fd);
    else
        handleClient(fd, event.events); // Otherwise, handle activity on an existing client socket, such as reading a request or sending a response
}

bool Server::isListenSocket(int fd)
{
    for (size_t i = 0; i < listen_fds.size(); i++)
    {
        if (listen_fds[i] == fd)
            return true;
    }
    return false;
}

void Server::acceptClient(int listen_fd)
{
    int client_fd = accept(listen_fd, NULL, NULL); // Accept a new incoming connection on the listening socket, which returns a new file descriptor for the client socket. The client's address information is not needed in this case, so NULL is passed for the address and its length.
    
    if (client_fd < 0) {
        // ✅ Check error reason
        if (errno == EMFILE || errno == ENFILE) {
            std::cerr << "Too many open files - rejecting connection" << std::endl;
        }
        return;
    }

    fcntl(client_fd, F_SETFL, O_NONBLOCK); // Set the client socket to non-blocking mode, allowing for asynchronous I/O operations without blocking the server's main loop without it the server would block on read/write operations, preventing it from handling other clients or accepting new connections until the current operation completes.

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = client_fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) 
    {
        std::cerr << "Failed to add client to epoll" << std::endl;
        close(client_fd);  // ✅ Close if can't add to epoll
        throw std::runtime_error("epoll_ctl client add failed");
    }

    clients[client_fd] = new Client(client_fd);

    std::cout << "New client connected: " << client_fd << std::endl;
}

void Server::closeClient(int fd)
{
    if (clients.find(fd) != clients.end()) // Check if the client exists in the clients map and if so, delete the Client object and remove it from the map to free up resources associated with that client
    {
        delete clients[fd]; // Free the memory allocated for the Client object associated with the client file descriptor
        clients.erase(fd);  // Remove the client from the clients map to free up resources associated with that client
    }
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL); // Unregister the client socket from the epoll instance to stop monitoring it for events, which is necessary when closing the client connection to prevent the server from trying to access an invalid file descriptor
    close(fd);
    std::cout << "Client disconnected: " << fd << std::endl;
}

void Server::handleClient(int client_fd, uint32_t events)
{
    // ✅ Check for errors first
    if (events & EPOLLERR)
    {
        std::cerr << "Error event on client " << client_fd << std::endl;
        closeClient(client_fd);
        return;
    }

    if (events & EPOLLHUP)
    {
        std::cout << "Client hung up: " << client_fd << std::endl;
        closeClient(client_fd);
        return;
    }
    if (events & EPOLLIN)
        handleRead(client_fd);
    else if (events & EPOLLOUT)
        handleWrite(client_fd);
}


void Server::processRequest(int client_fd)
{
    Client *client = clients[client_fd];

    // 🔥 MOCK RESPONSE
    client->response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 13\r\n"
        "\r\n"
        "Hello World!\n";

    // switch to write mode
    struct epoll_event event;
    event.events = EPOLLOUT;
    event.data.fd = client_fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &event) == -1)
        throw std::runtime_error("epoll_ctl MOD failed");
}

void Server::handleWrite(int client_fd)
{
    Client *client = clients[client_fd];

    if (client->response.empty()) {
        closeClient(client_fd);
        return;
    }

    int sent = send(client_fd,//send is a system call used to send data over a socket. It takes the client file descriptor, a pointer to the data to be sent (in this case, the response string), the length of the data, and flags (set to 0 for default behavior). The return value indicates how many bytes were actually sent, which may be less than the total length of the response if the client's receive buffer is full or if an error occurs.
                    client->response.c_str(),
                    client->response.size(),
                    0);

    if (sent <= 0)//<= 0 means error or connection closed by client and < 0 means an error occurred during sending, while 0 means the client has closed the connection. In either case, the server should close the client connection to free up resources and prevent further attempts to send data to a client that is no longer connected.
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK) //EAGAIN and EWOULDBLOCK indicate that the socket is not currently ready for writing, which can happen in non-blocking mode if the client's receive buffer is full. In this case, the server should not close the connection but instead wait for the next opportunity to send data when the socket becomes writable again.
        {
            return;  // ✅ Try again next time
        }
        closeClient(client_fd);
        return;
    }

    if (sent > 0) {
        // ✅ Remove sent data
        client->response = client->response.substr(sent);//substr is used to create a new string that contains the remaining part of the response after the sent portion has been removed. This is necessary because the send system call may not send the entire response in one call, especially if the response is large or if the client's receive buffer is full. By updating the response string to only contain the unsent portion, the server can attempt to send the remaining data in subsequent calls to handleWrite until the entire response has been sent.
        
        if (client->response.empty()) {
            // ✅ All sent, close client
            closeClient(client_fd);
        } else {
            // ✅ More to send, keep in EPOLLOUT mode
            struct epoll_event event;
            event.events = EPOLLOUT;
            event.data.fd = client_fd;
            epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &event);
        }
    }
}

void Server::handleRead(int client_fd)
{
    Client *client = clients[client_fd];
    char buffer[4096];
    int bytes = recv(client_fd, buffer, sizeof(buffer), 0);

    if (bytes <= 0)
    {
        closeClient(client_fd);
        return;
    }

    client->buffer.append(buffer, bytes);

    // 🔥 TEMP MOCK: detect end of headers only
    if (client->buffer.find("\r\n\r\n") != std::string::npos)
    {
        processRequest(client_fd);
        client->buffer.clear();
    }
}