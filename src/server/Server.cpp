#include "Server.hpp"


Server::Server(Config &config)
{
	this->config = config;
	epoll_fd = -1; // Initialize epoll_fd to an invalid value
}

// Server.cpp
Server::~Server()
{
	// Close all client connections
	for (std::map<int, Client *>::iterator it = clients.begin();
		 it != clients.end(); ++it)
	{
		int fd = it->first;
		delete it->second;
		close(fd);
	}
	clients.clear();

	// Remove all listen sockets from epoll
	for (size_t i = 0; i < listen_fds.size(); i++)
	{
		epoll_ctl(epoll_fd, EPOLL_CTL_DEL, listen_fds[i], NULL);
		close(listen_fds[i]);
	}
	listen_fds.clear();

	
	if (epoll_fd != -1)
	{
		close(epoll_fd);
	}

	std::cout << "Server shut down cleanly" << std::endl;
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
            throw std::runtime_error("epoll_wait failed");
        }

		for (int i = 0; i < nfds; i++)
		{
			try
			{
				handleEvent(events[i]); // Handle each event returned by epoll_wait, which could be a new incoming connection or activity on an existing client socket
			}
			catch (const BadRequestException &e)
			{
				std::string res = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n"; // Prepare a simple HTTP 400 Bad Request response to send back to the client when a bad request is encountered
				std::cerr << "Bad request: " << e.what() << std::endl;
				send(events[i].data.fd, res.c_str(), res.length(), 0); // Send a simple HTTP 400 Bad Request response to the client to inform them of the issue with their request before closing the connection
				closeClient(events[i].data.fd); // Close the client connection if a bad request is encountered to free up resources and prevent further issues with that client
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








