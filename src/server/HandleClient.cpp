#include"Server.hpp"

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
            EpollData* data = new EpollData;
            data->fd = client_fd;
            data->type = CLIENT;
            data->client = client;
            event.data.ptr = data;
            epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &event);
        }
    }
}

void Server::handleRead(int client_fd)
{
	Client *client = clients[client_fd];
	char buffer[4096];
	int bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

	if (bytes > 0)
	{
		buffer[bytes] = '\0'; // Null-terminate the buffer to safely convert it to a string
		client->request.append_to_buffer(buffer); // Append the received data to the client's request buffer for
	}
	else if (bytes == 0)//this means the client has closed the connection, so the server should close the client connection to free up resources and prevent further attempts to read from a client that is no longer connected.
		client->request.validate();// Validate the request once the client has finished sending data (indicated by recv returning 0), which may involve checking the completeness and correctness of the request before processing it further. If the request is valid, the server can proceed to generate a response based on the request data.
	else
	{
	   closeClient(client_fd);
		return ;
	}

	if (client->request.is_finished())
		processRequest(client_fd); // Process the client's request once it is fully received and validated, which may involve generating a response based on the request data and preparing it to be sent back to the client.
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
    EpollData* data = new EpollData;
    data->fd = client_fd;
    data->type = CLIENT;
    data->client = client;
    event.data.ptr = data;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &event) == -1)
        throw std::runtime_error("epoll_ctl MOD failed");
}