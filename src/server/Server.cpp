#include"Server.hpp"

Server::Server(Config &config)
{
    this->config = config;
    epoll_fd = -1; // Initialize epoll_fd to an invalid value


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
        if(sock<0)
           throw std::runtime_error("Failed to create socket");
        //allow this socket to reuse the address/port even if it’s still in TIME_WAIT
        int opt = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));// Allow reuse of the address

        fcntl(sock, F_SETFL, O_NONBLOCK);// Set the socket to non-blocking mode

        sockaddr_in addr;
        addr.sin_family = AF_INET;// IPv4
        addr.sin_port = htons(port);// Convert port to network byte order

        if (ip == "0.0.0.0")
            addr.sin_addr.s_addr = INADDR_ANY;
        else
            inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);// Convert IP address from string to binary form

        //binding means associating the socket with a specific IP address and port number on the local machine
        if(bind(sock, (sockaddr*)&addr, sizeof(addr))<0)
            throw std::runtime_error("Failed to bind socket to address");
        // Start listening for incoming connections on the socket with a backlog of SOMAXCONN (maximum allowed by the system)
        if(listen(sock, SOMAXCONN)<0)
            throw std::runtime_error("Failed to listen on socket");
        //what turns a socket into a server socket that can accept connections
        listen_fds.push_back(sock);

        std::cout << "Listening on " << ip << ":" << port << std::endl;
    }
}
void Server::initEpoll()
{
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
        throw std::runtime_error("epoll_create1 failed");

    struct epoll_event event;
    event.events = EPOLLIN;

    for (size_t i = 0; i < listen_fds.size(); i++)
    {
        event.data.fd = listen_fds[i];// Store the file descriptor in the event data for later identification

        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fds[i], &event) == -1)// Register the listening socket with the epoll instance to monitor for incoming connections
            throw std::runtime_error("epoll_ctl ADD failed");
    }
}
