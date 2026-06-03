#include "Server.hpp"

void Server::handleDeleteFile(int client_fd, const RouteInfo &route)
{
    Client *client = clients[client_fd];
    std::string file_path = route.file_path;

    if (unlink(file_path.c_str()) != 0)
    {
        if (errno == EACCES)
            client->response = buildErrorResponse(403, "Forbidden");
        else if (errno == ENOENT)
            client->response = buildErrorResponse(404, "Not Found");
        else
            client->response = buildErrorResponse(500, "Internal Server Error");

        return;
    }
    client->response = "HTTP/1.1 204 No Content\r\n";
    client->response += "Content-Length: 0\r\n";
    client->response += "Connection: close\r\n";
    client->response += "\r\n";

    std::cout << "File deleted: " << file_path << std::endl;
}