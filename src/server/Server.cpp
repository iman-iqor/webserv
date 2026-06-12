#include "Server.hpp"
volatile sig_atomic_t g_shutdown = 0; // This flag will be set to true when a shutdown signal is received, allowing the server's main loop to exit 


Server::Server(Config &config)
{
	this->config = config;
	router = new Router(&this->config);
	epoll_fd = -1;
}

// Close all client connections and free resources associated with them
// Remove all listen sockets from epoll
Server::~Server()
{
	for (std::map<int, Client *>::iterator it = clients.begin();
		 it != clients.end(); ++it)
	{
		int fd = it->first;
		delete it->second;
		close(fd);
	}
	clients.clear();

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
	if (router)
		delete router;

	std::cout << "Server shut down cleanly" << std::endl;
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
		EpollData *data = new EpollData();
		data->fd = listen_fds[i];
		data->type = SERVER;
		data->client = NULL;
		epoll_data[listen_fds[i]] = data;
		event.data.ptr = data;
		if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fds[i], &event) == -1)
			throw std::runtime_error("epoll_ctl ADD failed");
	}
}

void Server::handleClientError(Client *client, const HttpException &e)
{
	if (!client)
		return;
	//find the server block fo this cliet
	ServerBlock *server_block = NULL;
	if(client->listen_fd != -1)
	{
		if(fd_to_servers.find(client->listen_fd) != fd_to_servers.end())
		{
			if(fd_to_servers[client->listen_fd].size()>0)
				server_block = fd_to_servers[client->listen_fd][0];
		}
	}

	RouteInfo route_info;
	route_info.location = NULL;
	route_info.http_status = e.statusCode;
	route_info.status_message = e.statusMessage;

	std::string error_page_path = "";
	
	if(server_block)
	{
		Router router(&config);
		error_page_path = router.resolveErrorPage(e.statusCode,server_block);
	}

	if(!error_page_path.empty())
	{
		route_info.action = SERVE_FILE;
		route_info.file_path = error_page_path;
	}
	else if (e.statusCode == 405)
    	route_info.action = ERROR_405;
	else if (e.statusCode == 400)
    	route_info.action = ERROR_400;
	else if (e.statusCode == 500)
    	route_info.action = ERROR_500;
	else if (e.statusCode == 413)
		route_info.action = ERROR_413;
	else if(e.statusCode == 501)
		route_info.action = ERROR_501;
	else if(e.statusCode == 411)
		route_info.action = ERROR_411;
	else
    	route_info.action = ERROR_404;

	Response response(*this);
	response.handleResponse(route_info,server_block ? server_block->error_pages : std::map<int,std::string>(),client);

	std::string res = response.build();
	send(client->fd, res.c_str(), res.size(), 0);
}

void Server::start()
{
	struct epoll_event events[1024];

	while (!g_shutdown)
	{
		int nfds = epoll_wait(epoll_fd, events, 1024, -1);
		if (nfds < 0)
		{
			if (errno == EINTR)
				break;
			throw std::runtime_error("epoll_wait failed");
		}

		for (int i = 0; i < nfds; i++)
		{
			try
			{
				handleEvent(events[i]);
				
			}
			catch (const HttpException &e)
			{
				// events[i].data.fd is not valid when we stored a pointer in data.ptr.
				// Retrieve the EpollData pointer and use its fd to find the client.
				int client_fd = -1;
				std::cerr << RED << "HTTP error: " << e.statusCode << " " << e.statusMessage << ": " << e.what() << RESET << std::endl;
				EpollData *ed = NULL;
				if (events[i].data.ptr) {
					ed = static_cast<EpollData *>(events[i].data.ptr);
					if (ed->type == CGI_PIPE) {
						std::cout << RED << "Error occurred in CGI execution for client " << ed->client->fd << RESET << std::endl;
						client_fd = ed->client->fd;
					}
					else if (ed->type == CLIENT) {
						std::cout << RED << "Error occurred while handling client " << ed->fd << RESET << std::endl;
						client_fd = ed->fd;
					}
				}
				Client *client_ptr = NULL;
				if (client_fd != -1 && clients.find(client_fd) != clients.end())
					client_ptr = clients[client_fd];
				handleClientError(client_ptr, e);
				// if (client_fd != -1)
				// 	closeClient(client_fd);
			}
			catch (const std::exception &e)
			{
				std::cerr << "Error handling event: " << e.what() << std::endl;
			}
		}
	}
}

void Server::handleEvent(struct epoll_event &event)
{
	EpollData *data = (EpollData *)event.data.ptr;
	int fd = data->fd;

	if (data->type == SERVER)
		acceptClient(fd);
	else if (data->type == CLIENT)
		handleClient(data, event.events);
	else if (data->type == CGI_PIPE)
		handleCGI(data, event.events);
}

void Server::closeClient(int fd)
{
	if (clients.find(fd) != clients.end())
	{
		delete clients[fd];
		clients.erase(fd);
	}
	if (epoll_data.find(fd) != epoll_data.end())
	{
		delete epoll_data[fd];
		epoll_data.erase(fd);
	}
	epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
	close(fd);
	std::cout << "Client disconnected: " << fd << std::endl;
}
