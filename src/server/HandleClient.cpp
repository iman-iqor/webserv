#include"Server.hpp"

void Server::handleClient(EpollData* data, uint32_t events)
{
    int client_fd = data->fd;
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
        handleRead(data->client);
    else if (events & EPOLLOUT)
        handleWrite(data->client);
}
void Server::handleWrite(Client *client)
{
    // Client *client = clients[client_fd];

    if (client->response.empty()) {
        closeClient(client->fd);
        return;
    }

    int sent = send(client->fd,//send is a system call used to send data over a socket. It takes the client file descriptor, a pointer to the data to be sent (in this case, the response string), the length of the data, and flags (set to 0 for default behavior). The return value indicates how many bytes were actually sent, which may be less than the total length of the response if the client's receive buffer is full or if an error occurs.
                    client->response.c_str(),
                    client->response.size(),
                    0);

    if (sent <= 0)//<= 0 means error or connection closed by client and < 0 means an error occurred during sending, while 0 means the client has closed the connection. In either case, the server should close the client connection to free up resources and prevent further attempts to send data to a client that is no longer connected.
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK) //EAGAIN and EWOULDBLOCK indicate that the socket is not currently ready for writing, which can happen in non-blocking mode if the client's receive buffer is full. In this case, the server should not close the connection but instead wait for the next opportunity to send data when the socket becomes writable again.
        {
            return;  // ✅ Try again next time
        }
        closeClient(client->fd);
        return;
    }

    if (sent > 0) {
        // ✅ Remove sent data
        client->response = client->response.substr(sent);//substr is used to create a new string that contains the remaining part of the response after the sent portion has been removed. This is necessary because the send system call may not send the entire response in one call, especially if the response is large or if the client's receive buffer is full. By updating the response string to only contain the unsent portion, the server can attempt to send the remaining data in subsequent calls to handleWrite until the entire response has been sent.
        
        if (client->response.empty()) {
            // ✅ All sent, close client
            closeClient(client->fd);
        } else {
            // ✅ More to send, keep in EPOLLOUT mode
            struct epoll_event event;
            event.events = EPOLLOUT;
            EpollData* data = new EpollData;
            data->fd = client->fd;
            data->type = CLIENT;
            data->client = client;
            event.data.ptr = data;
            epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client->fd, &event);
        }
    }
}

void Server::handleRead(Client *client)
{
	// Client *client = clients[client_fd];
	char buffer[4096];
	int bytes = recv(client->fd, buffer, sizeof(buffer) - 1, 0);

	if (bytes > 0)
	{
		buffer[bytes] = '\0'; // Null-terminate the buffer to safely convert it to a string
		client->request.append_to_buffer(buffer); // Append the received data to the client's request buffer for
	}
	else
	{
	   closeClient(client->fd);
       //cleanup and close connection if recv returns 0 (client closed connection) or -1 (error)
		return ;
	}
	if (client->request.is_finished())
		processRequest(client->fd); // Process the client's request once it is fully received and validated, which may involve generating a response based on the request data and preparing it to be sent back to the client.
    
}

std::string intToString(size_t n)
{
    std::stringstream s;
    s<<n;
    return s.str();
}

void Server::processRequest(int client_fd) {
    Client *client = clients[client_fd];
    
    // STEP 1: Get the server block for this client
    ServerBlock* server_block = NULL;
    if (fd_to_servers.find(client->listen_fd) != fd_to_servers.end()) {
        if (fd_to_servers[client->listen_fd].size() > 0) {
            server_block = fd_to_servers[client->listen_fd][0];
        }
    }
    
    if (!server_block) {
        // No server config found
        client->response = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
        return;
    }
    
    // ✅ STEP 2: USE ROUTER TO DETERMINE ACTION
    RouteInfo route = router->route(client->request, server_block);
    
    // ✅ STEP 3: HANDLE BASED ON ROUTE ACTION
    switch (route.action) {
        
        case SERVE_FILE: {
            // Read file and send it
            std::ifstream file(route.file_path.c_str());
            std::string file_content((std::istreambuf_iterator<char>(file)),
                                    std::istreambuf_iterator<char>());
            
            client->response = "HTTP/1.1 200 OK\r\n";
            client->response += "Content-Length: " + intToString(file_content.length()) + "\r\n";
            client->response += "Connection: keep-alive\r\n";
            client->response += "\r\n";
            client->response += file_content;
            break;
        }
        
        case DIRECTORY_LISTING: {
            // Generate HTML directory listing
            std::string html = "<html><body><h1>Directory Listing</h1><ul>";
            // TODO: Generate list of files
            html += "</ul></body></html>";
            
            client->response = "HTTP/1.1 200 OK\r\n";
            client->response += "Content-Type: text/html\r\n";
            client->response += "Content-Length: " + intToString(html.length()) + "\r\n";
            client->response += "Connection: keep-alive\r\n";
            client->response += "\r\n";
            client->response += html;
            break;
        }
        
        case REDIRECT: {
            client->response = "HTTP/1.1 301 Moved Permanently\r\n";
            client->response += "Location: " + route.redirect_url + "\r\n";
            client->response += "Content-Length: 0\r\n";
            client->response += "Connection: keep-alive\r\n";
            client->response += "\r\n";
            break;
        }
        
        case EXECUTE_CGI: {
            // TODO: Execute CGI script
            client->response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
            break;
        }
        
        case ERROR_404: {
            std::string error_body = "<html><body><h1>404 Not Found</h1></body></html>";
            client->response = "HTTP/1.1 404 Not Found\r\n";
            client->response += "Content-Type: text/html\r\n";
            client->response += "Content-Length: " + intToString(error_body.length()) + "\r\n";
            client->response += "Connection: keep-alive\r\n";
            client->response += "\r\n";
            client->response += error_body;
            break;
        }
        
        case ERROR_403: {
            std::string error_body = "<html><body><h1>403 Forbidden</h1></body></html>";
            client->response = "HTTP/1.1 403 Forbidden\r\n";
            client->response += "Content-Type: text/html\r\n";
            client->response += "Content-Length: " + intToString(error_body.length()) + "\r\n";
            client->response += "Connection: keep-alive\r\n";
            client->response += "\r\n";
            client->response += error_body;
            break;
        }
        
        case ERROR_405: {
            std::string error_body = "<html><body><h1>405 Method Not Allowed</h1></body></html>";
            client->response = "HTTP/1.1 405 Method Not Allowed\r\n";
            client->response += "Content-Type: text/html\r\n";
            client->response += "Content-Length: " + intToString(error_body.length()) + "\r\n";
            client->response += "Allow: GET, POST, DELETE\r\n";
            client->response += "Connection: keep-alive\r\n";
            client->response += "\r\n";
            client->response += error_body;
            break;
        }
        
        default: {
            client->response = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
        }
    }
    
    // STEP 4: Switch to write mode
    struct epoll_event event;
    event.events = EPOLLOUT;
    event.data.fd = client_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &event);
}