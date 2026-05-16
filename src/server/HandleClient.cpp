#include"Server.hpp"
#include"../http/CgiHandler.hpp"
#include <signal.h>
#include <sys/wait.h>

void handle_cgi_stdin(Client *client)
{
    if (!client->cgi_state || client->cgi_state->stdin_fd < 0)
        return;
    
    const std::string &body = client->request.get_body();
    size_t remaining = body.size() - client->cgi_state->body_sent;
    
    std::cout << "[CGI_STDIN] body_size=" << body.size() << " sent=" << client->cgi_state->body_sent 
              << " remaining=" << remaining << std::endl;
    
    if (remaining > 0)
    {
        int written = write(client->cgi_state->stdin_fd,
                           body.c_str() + client->cgi_state->body_sent,
                           remaining);
        if (written > 0)
        {
            client->cgi_state->body_sent += written;
            std::cout << "[CGI_STDIN] wrote " << written << " bytes, total=" << client->cgi_state->body_sent << std::endl;
        }
        else if (written < 0 && (errno != EAGAIN && errno != EWOULDBLOCK))
        {
            std::cerr << "[CGI_STDIN] write error, closing stdin_fd" << std::endl;
            close(client->cgi_state->stdin_fd);
            client->cgi_state->stdin_fd = -1;
        }
    }
    else if (client->cgi_state->body_sent == body.size())
    {
        std::cout << "[CGI_STDIN] All body sent (" << body.size() << " bytes), closing stdin_fd" << std::endl;
        close(client->cgi_state->stdin_fd);
        client->cgi_state->stdin_fd = -1;
    }
}

void handle_cgi_stdout(Client *client)
{
    if (!client->cgi_state || client->cgi_state->stdout_fd < 0)
        return;
    
    char buffer[4096];
    int bytes = read(client->cgi_state->stdout_fd, buffer, sizeof(buffer) - 1);
    
    std::cout << "[CGI_STDOUT] read=" << bytes << " total_collected=" << client->cgi_state->cgi_output.size() << std::endl;
    
    if (bytes > 0)
    {
        client->cgi_state->cgi_output.append(buffer, bytes);
        std::cout << "[CGI_STDOUT] appended, now total=" << client->cgi_state->cgi_output.size() << std::endl;
    }
    else if (bytes == 0)
    {
        std::cout << "[CGI_STDOUT] EOF, closing stdout_fd" << std::endl;
        close(client->cgi_state->stdout_fd);
        client->cgi_state->stdout_fd = -1;
        
        int status = 0;
        pid_t wpid = waitpid(client->cgi_state->pid, &status, WNOHANG);
        std::cout << "[CGI_STDOUT] waitpid(" << client->cgi_state->pid << ") returned " << wpid << " status=" << status << std::endl;
        
        if (wpid == client->cgi_state->pid)
        {
            std::cout << "[CGI_STDOUT] Child exited! Building response." << std::endl;
            std::ostringstream oss;

            oss << "HTTP/1.1 200 OK\r\n";
            oss << "Content-Type: text/plain\r\n";
            oss << "Content-Length: "  << client->cgi_state->cgi_output.size() << "\r\n";
            oss << "\r\n";
            oss << client->cgi_state->cgi_output;
            client->response = oss.str();
            client->ready_to_send = true;
            std::cout << "[CGI_STDOUT] ready_to_send=true, response_size=" << client->response.size() << std::endl;
            client->cleanup_cgi();
        }
        else
            std::cout << "[CGI_STDOUT] Child still running (wpid=" << wpid << ")" << std::endl;
    }
    else if (bytes < 0 && (errno != EAGAIN && errno != EWOULDBLOCK))
    {
        std::cerr << "[CGI_STDOUT] read error " << errno << ", closing stdout_fd" << std::endl;
        close(client->cgi_state->stdout_fd);
        client->cgi_state->stdout_fd = -1;
    }
}

void Server::handleClient(EpollData* data, uint32_t events)
{
    int client_fd = data->fd;
    
    // Handle CGI pipe events FIRST before generic EPOLLERR/EPOLLHUP
    // to prevent pipe close from being treated as client disconnect.
    if (data->type == CGI_PIPE)
    {
        std::cout << "[HANDLE_CGI_PIPE] event fd=" << data->fd << " events=" << events << " (EPOLLOUT=" << EPOLLOUT << " EPOLLIN=" << EPOLLIN << " EPOLLHUP=" << EPOLLHUP << ")" << std::endl;
        Client *client = data->client;
        if (!client)
            return;
        
        if (events & EPOLLOUT)
        {
            std::cout << "[HANDLE_CGI_PIPE] EPOLLOUT event, calling handle_cgi_stdin" << std::endl;
            handle_cgi_stdin(client);
        }
        if (events & EPOLLIN)
        {
            std::cout << "[HANDLE_CGI_PIPE] EPOLLIN event, calling handle_cgi_stdout" << std::endl;
            handle_cgi_stdout(client);
        }
        
        // After reading, check for EPOLLHUP to signal EOF and allow response building
        if (events & EPOLLHUP)
        {
            std::cout << "[HANDLE_CGI_PIPE] EPOLLHUP on CGI pipe fd=" << data->fd << std::endl;
            if (data->fd == client->cgi_state->stdout_fd && client->cgi_state->stdout_fd >= 0)
            {
                std::cout << "[HANDLE_CGI_PIPE] Triggering EOF handling for stdout" << std::endl;
                close(client->cgi_state->stdout_fd);
                client->cgi_state->stdout_fd = -1;
                
                int status = 0;
                pid_t wpid = waitpid(client->cgi_state->pid, &status, WNOHANG);
                std::cout << "[HANDLE_CGI_PIPE] waitpid(" << client->cgi_state->pid << ") returned " << wpid << std::endl;
                
                if (wpid == client->cgi_state->pid)
                {
                    std::cout << "[HANDLE_CGI_PIPE] Child exited! Building response." << std::endl;
                    std::ostringstream oss;
                    oss << "HTTP/1.1 200 OK\r\n";
                    oss << "Content-Type: text/plain\r\n";
                    oss << "Content-Length: " << client->cgi_state->cgi_output.size() << "\r\n";
                    oss << "\r\n";
                    oss << client->cgi_state->cgi_output;
                    client->response = oss.str();
                    client->ready_to_send = true;
                    std::cout << "[HANDLE_CGI_PIPE] ready_to_send=true, response_size=" << client->response.size() << std::endl;
                    
                    // Deregister and delete the CGI pipe EpollData
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, data->fd, NULL);
                    delete data;
                    
                    client->cleanup_cgi();
                }
            }
        }
        
        if (client->ready_to_send)
        {
            std::cout << "[HANDLE_CGI_PIPE] ready_to_send=true, re-registering client fd=" << client->fd << " for EPOLLOUT" << std::endl;
            struct epoll_event event;
            event.events = EPOLLOUT;
            EpollData* wr_data = new EpollData;
            wr_data->fd = client->fd;
            wr_data->type = CLIENT;
            wr_data->client = client;
            event.data.ptr = wr_data;
            if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client->fd, &event) == -1)
            {
                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client->fd, &event);
            }
        }
        return;
    }
    
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

	std::cout << "[HANDLE_READ] fd=" << client->fd << " bytes=" << bytes << std::endl;

	if (bytes > 0)
	{
		buffer[bytes] = '\0'; // Null-terminate the buffer to safely convert it to a string
		client->request.append_to_buffer(buffer); // Append the received data to the client's request buffer for
		std::cout << "[HANDLE_READ] appended " << bytes << " bytes to request" << std::endl;
	}
	else
	{
	   std::cout << "[HANDLE_READ] recv=" << bytes << ", closing client" << std::endl;
	   closeClient(client->fd);
       //cleanup and close connection if recv returns 0 (client closed connection) or -1 (error)
		return ;
	}
	if (client->request.is_finished())
	{
		std::cout << "[HANDLE_READ] request finished! calling processRequest" << std::endl;
		processRequest(client->fd); // Process the client's request once it is fully received and validated, which may involve generating a response based on the request data and preparing it to be sent back to the client.
	}
    else
        std::cout << "[HANDLE_READ] request not finished yet" << std::endl;
}

void Server::processRequest(int client_fd)
{
    Client *client = clients[client_fd];

    try
    {
        std::cout << BOLD_YELLOW << "[SERVER]" << RESET << " Processing request from client " << client_fd << std::endl;
        client->cgi_state = new Client::CgiState;
        client->cgi_state->pid = 0;
        client->cgi_state->stdin_fd = -1;
        client->cgi_state->stdout_fd = -1;
        client->cgi_state->body_sent = 0;
        
        std::string bin_path = "/usr/bin/python3";
        std::string cgi_path = "test.py";
        
        CgiProcessResult result = cgi_start(client, cgi_path, bin_path, epoll_fd);
        
        client->cgi_state->pid = result.pid;
        client->cgi_state->stdin_fd = result.request_write_fd;
        client->cgi_state->stdout_fd = result.response_read_fd;
        
        std::cout << "[SERVER] CGI process forked: pid=" << result.pid 
                  << " stdin_fd=" << result.request_write_fd 
                  << " stdout_fd=" << result.response_read_fd << std::endl;
        
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
    }
    catch (const std::exception &e)
    {
        std::cerr << "CGI error: " << e.what() << std::endl;
        client->response =
            "HTTP/1.1 500 Internal Server Error\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 21\r\n"
            "\r\n"
            "Internal Server Error\n";
        client->ready_to_send = true;
        
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