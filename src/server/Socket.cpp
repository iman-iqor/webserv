#include"Server.hpp"


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
        //AF_INET specifies that the socket will use the IPv4 protocol, 
        //SOCK_STREAM indicates that it will be a TCP socket (as opposed to a datagram socket for UDP), and 0 means to use the default protocol for the given socket type (which is TCP for SOCK_STREAM). The return value is a file descriptor for the newly created socket, which can be used in subsequent system calls to configure and manage the socket. If the socket creation fails, it returns -1, which is checked in the code to throw an exception if the socket cannot be created successfully.
        if (sock < 0)
            throw std::runtime_error("Failed to create socket");
        
        //set the socket options to allow reuse of the address and port, which is useful for quickly restarting the server without waiting for the operating system to release the previous socket
        int opt = 1;
        if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
            throw std::runtime_error("Failed to set socket options");

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
            throw
                std::runtime_error("bind failed on " + ip + ":" + std::string("bind failed: ") + strerror(errno));


        // Start listening for incoming connections on the socket with a backlog of SOMAXCONN (maximum allowed by the system)
        if (listen(sock, SOMAXCONN) < 0)
            throw std::runtime_error("Failed to listen on socket");
            
        // what turns a socket into a server socket that can accept connections
        listen_fds.push_back(sock);

        std::cout << "Listening on " << ip << ":" << port << std::endl;
    }
}