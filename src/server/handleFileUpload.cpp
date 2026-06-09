#include "Server.hpp"
#include <ctime>

/*
The second parameter specifies HOW the file should be opened.
Without it, the file is opened in normal text mode.
Binary mode is important when storing raw bytes like:
images
PDFs
videos
uploaded files
CGI output
*/

void Server::handleFileUpload(int client_fd, const RouteInfo &route, Request &request)
{
    std::cout << "inside handle file upload" << std::endl;
    Client *client = clients[client_fd];

    // get the directory i will upload in
    std::string upload_dir = route.upload_dir;
    if (upload_dir.empty() || upload_dir[upload_dir.length() - 1] != '/')
        upload_dir += "/";

    // generate a filename for the uploaded file and if it s empty i will create a secured one
    std::string filename = router->generateUploadFilename(request, route.location);

    std::string full_path = upload_dir + filename;

    std::string body = request.get_body();

    std::ofstream file(full_path.c_str(), std::ios::binary);
    if (!file.is_open())
    {
        std::string body = "<html><body><h1>500 Internal Server Error</h1></body></html>";
        client->response = "HTTP/1.1 500 Internal Server Error\r\n";
        client->response += "Content-Type: text/html\r\n";
        client->response += "Content-Length: " + intToString(body.length()) + "\r\n";
        client->response += "\r\n";
        client->response += body;

        return;
    }

    // write the body to file
    file.write(body.c_str(), static_cast<std::streamsize>(body.length()));
    if (file.fail())
    {
        file.close();

        std::string body = "<html><body><h1>500 Failed to write file</h1></body></html>";
        client->response = "HTTP/1.1 500 Failed to write file\r\n";
        client->response += "Content-Type: text/html\r\n";
        client->response += "Content-Length: " + intToString(body.length()) + "\r\n";
        client->response += "\r\n";
        client->response += body;

        return;
    }
    file.close();

    std::stringstream ss;
    ss << "File uploaded succesfuly: " + filename;
    std::string response_body = ss.str();

    std::stringstream content_len;
    content_len << response_body.length();
    client->response = "HTTP/1.1 201 Created\r\n";
    client->response += "Content-Type: text/plain\r\n";
    client->response += "Content-Length: " + content_len.str() + "\r\n";
    client->response += "Connection: close\r\n";
    client->response += "\r\n";
    client->response += response_body;
}

void Server::switchToWrite(int client_fd)
{
    struct epoll_event event;
    event.events = EPOLLOUT;
    event.data.fd = client_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &event);
} // build success response(201 created)