#include "Server.hpp"
#include "../http/CgiHandler.hpp"

void Server::handleClient(EpollData *data, uint32_t events)
{
    int client_fd = data->fd;
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
    if (client->response.empty())
    {
        closeClient(client->fd);
        return;
    }

    int sent = send(client->fd,
                    client->response.c_str(),
                    client->response.size(),
                    0);

    if (sent <= 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;

        closeClient(client->fd);
        return;
    }

    if (sent > 0)
    {

        client->response = client->response.substr(sent);

        if (client->response.empty())
        {
            closeClient(client->fd);
        }
        else
        {

            struct epoll_event event;
            event.events = EPOLLOUT;
            EpollData *data = new EpollData(client->fd, CLIENT, client);
            event.data.ptr = data;
            epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client->fd, &event);
        }
    }
}

void Server::handleRead(Client *client)
{

    char buffer[4096];
    ssize_t bytes = recv(client->fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes > 0)
    {
        buffer[bytes] = '\0';
        client->request.append_request(buffer, bytes);
    }
    else
    {
        closeClient(client->fd);

        return;
    }
    if (client->request.is_finished())
        processRequest(client->fd);
}

std::string Server::intToString(size_t n)
{
    std::stringstream s;
    s << n;
    return s.str();
}

void Server::processRequest(int client_fd)
{
    Client *client = clients[client_fd];

    ServerBlock *server_block = NULL;
    if (fd_to_servers.find(client->listen_fd) != fd_to_servers.end())
    {
        if (fd_to_servers[client->listen_fd].size() > 0)
        {
            server_block = fd_to_servers[client->listen_fd][0];
        }
    }

    if (!server_block)
    {

        client->response = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
        return;
    }

    RouteInfo route = router->route(client->request, server_block);
    std::cout<<"Route action: "<<route.action<<" for client "<<client_fd<<std::endl;
    std::cout<<"Route file path: "<<route.file_path<<" for client "<<client_fd<<std::endl;  

    if (route.action == EXECUTE_CGI) {
        std::string bin_path = route.location->cgi[route.file_extension.substr(1)]; // remove the dot from extension
        if (bin_path.empty()) {
            throw BadGatewayException("No CGI handler found for file extension: " + route.file_extension);
        }
        // TODO: Add handle if the file extension is not supported for CGI execution
        CgiHandler::start(client, route.cgi_string, bin_path, epoll_fd, client->request.getHeaders());
        return;
    }

    Response res(*this);
    res.handleResponse(client_fd, route, server_block->error_pages, client);
    client->response = res.build();

    std::cout << "\033[32mPrepared response for client " << client_fd << ", switching to write mode\033[0m" << std::endl;
    struct epoll_event event;
    EpollData *data = new EpollData(client_fd, CLIENT, client);
    event.data.ptr = data;
    event.events = EPOLLOUT;
    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &event);
    std::cout << "\033[32mClient " << client_fd << " is now ready to send response\033[0m" << std::endl;
}