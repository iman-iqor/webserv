// ...existing code...
#include <arpa/inet.h>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <map>
#include <fstream>
#include <sys/stat.h>
#include <sstream>

#include "http/Request.hpp"

static const int PORT = 8080;
static const int MAX_EVENTS = 64;
static const int BACKLOG = 128;

static int setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return -1;
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        return -1;
    return 0;
}

static int createServerSocket(int port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
        return -1;

    int yes = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == -1)
        return -1;
    if (listen(fd, BACKLOG) == -1)
        return -1;
    if (setNonBlocking(fd) == -1)
        return -1;

    return fd;
}

struct Client
{
    Request request;
    std::string buffer;
};

void create_mock_server_block(ServerBlock &sb) {
    sb.listen_directives.push_back(std::make_pair("0.0.0.0", PORT));
    sb.listen_directives.push_back(std::make_pair("127.0.0.1", PORT));
    sb.server_names.push_back("localhost");
    sb.client_max_body_size = 1024 * 1024; // 1 MB
    sb.error_pages[400] = "/error/400.html";
    sb.error_pages[404] = "/error/404.html";
    sb.error_pages[500] = "/error/500.html";
    sb.error_pages[501] = "/error/501.html";
    sb.root = "src/www";
    
    // Create Location 1: root
    Location loc1;
    loc1.path = "/";
    loc1.root = "src/html";
    loc1.methods.push_back("GET");
    loc1.index = "index.html";
    loc1.autoindex = false;
    loc1.upload_path = "";
    loc1.return_url = "";
    loc1.return_code = 0;
    sb.locations.push_back(loc1);
    
    // Create Location 2: redirect
    Location loc2;
    loc2.path = "/redirect/";
    loc2.root = "src/html";
    loc2.methods.push_back("GET");
    loc2.index = "";
    loc2.autoindex = false;
    loc2.upload_path = "";
    loc2.return_url = "http://www.example.com";
    loc2.return_code = 301;
    sb.locations.push_back(loc2);
}

static std::string int_to_string(int num)
{
    std::ostringstream oss;
    oss << num;
    return oss.str();
}

static std::string size_to_string(size_t num)
{
    std::ostringstream oss;
    oss << num;
    return oss.str();
}

static std::string make_http_response(int status, const std::string &reason, const std::string &body, const std::string &contentType)
{
    std::string response = "HTTP/1.1 " + int_to_string(status) + " " + reason + "\r\n";
    response += "Content-Type: " + contentType + "\r\n";
    response += "Content-Length: " + size_to_string(body.size()) + "\r\n";
    response += "Connection: close\r\n\r\n";
    response += body;
    return response;
}

static bool read_file_to_string(const std::string &path, std::string &out)
{
    std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);
    if (!file)
        return false;
    out.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return true;
}

static std::string make_error_response_from_request(Request &request, int status, const std::string &reason)
{
    std::string body = reason;
    std::string contentType = "text/plain";

    ServerBlock *serverBlock = request.get_server_block();
    if (serverBlock)
    {
        std::map<int, std::string>::const_iterator it = serverBlock->error_pages.find(status);
        if (it != serverBlock->error_pages.end())
        {
            const std::string &configuredPath = it->second;
            std::string errorPage;

            bool loaded = read_file_to_string(configuredPath, errorPage);
            if (!loaded && !configuredPath.empty() && configuredPath[0] == '/')
                loaded = read_file_to_string(configuredPath.substr(1), errorPage);
            if (!loaded && !serverBlock->root.empty())
            {
                std::string rootedPath = serverBlock->root;
                if (!configuredPath.empty() && configuredPath[0] != '/')
                    rootedPath += "/";
                rootedPath += configuredPath;
                loaded = read_file_to_string(rootedPath, errorPage);
            }

            if (loaded)
            {
                body = errorPage;
                contentType = "text/html";
            }
        }
    }

    return make_http_response(status, reason, body, contentType);
}

static const Location *find_best_location(const ServerBlock &sb, const std::string &path)
{
    const Location *best = NULL;
    size_t best_len = 0;

    for (size_t i = 0; i < sb.locations.size(); ++i)
    {
        const Location &loc = sb.locations[i];
        if (path.find(loc.path) == 0 && loc.path.length() > best_len)
        {
            best = &loc;
            best_len = loc.path.length();
        }
    }

    return best;
}

static bool file_exists(const std::string &path)
{
    struct stat buffer;
    return stat(path.c_str(), &buffer) == 0;
}

static bool is_directory(const std::string &path)
{
    struct stat buffer;
    if (stat(path.c_str(), &buffer) != 0)
        return false;
    return S_ISDIR(buffer.st_mode);
}

static std::string resolve_request_path(const std::string &root, const std::string &uri, const std::string &index_file)
{
    std::string clean = uri;
    
    // Remove query string
    std::size_t q = clean.find('?');
    if (q != std::string::npos)
        clean = clean.substr(0, q);

    // Prevent directory traversal
    if (clean.find("..") != std::string::npos)
        return "";

    // Strip trailing slashes except for /
    if (clean.length() > 1 && clean[clean.length() - 1] == '/')
        clean.erase(clean.length() - 1);

    // Handle root case
    if (clean.empty() || clean == "/")
        return root + "/" + index_file;

    // Try exact path first
    std::string fullPath = root + clean;
    if (file_exists(fullPath))
    {
        if (is_directory(fullPath))
        {
            // Serve index file from directory
            return fullPath + "/" + index_file;
        }
        return fullPath;
    }

    // Try with .html extension
    fullPath = root + clean + ".html";
    if (file_exists(fullPath))
        return fullPath;

    // Try as directory with index file
    fullPath = root + clean;
    if (is_directory(fullPath))
        return fullPath + "/" + index_file;

    return root + clean + ".html"; // Return what would be tried last
}

int main()
{
    ServerBlock sb;
    create_mock_server_block(sb);
    std::signal(SIGPIPE, SIG_IGN);

    int serverFd = createServerSocket(PORT);
    if (serverFd == -1)
    {
        std::perror("server");
        return 1;
    }

    int epfd = epoll_create1(0);
    if (epfd == -1)
    {
        std::perror("epoll_create1");
        close(serverFd);
        return 1;
    }

    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = serverFd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, serverFd, &ev) == -1)
    {
        std::perror("epoll_ctl");
        close(epfd);
        close(serverFd);
        return 1;
    }

    std::map<int, Request> clients;
    epoll_event events[MAX_EVENTS];

    std::cout << "Listening on port " << PORT << std::endl;

    while (true)
    {
        int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (n == -1)
        {
            if (errno == EINTR)
                continue;
            std::perror("epoll_wait");
            break;
        }

        for (int i = 0; i < n; ++i)
        {
            int fd = events[i].data.fd;

            if (fd == serverFd)
            {
                while (true)
                {
                    sockaddr_in clientAddr;
                    socklen_t clientLen = sizeof(clientAddr);
                    int clientFd = accept(serverFd, reinterpret_cast<sockaddr *>(&clientAddr), &clientLen);
                    if (clientFd == -1)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break;
                        std::perror("accept");
                        break;
                    }
                    if (setNonBlocking(clientFd) == -1)
                    {
                        std::perror("fcntl");
                        close(clientFd);
                        continue;
                    }
                    epoll_event clientEv;
                    clientEv.events = EPOLLIN | EPOLLET;
                    clientEv.data.fd = clientFd;
                    if (epoll_ctl(epfd, EPOLL_CTL_ADD, clientFd, &clientEv) == -1)
                    {
                        std::perror("epoll_ctl add client");
                        close(clientFd);
                        continue;
                    }
                    Request req;
                    req.set_server_block(&sb);
                    clients[clientFd] = req;
                }
            }
            else
            {
                bool closed = false;
                while (true)
                {
                    try {

                        clients[fd].read_request(fd, &closed);
                        if (clients[fd].is_finished()) {
                            const Location *loc = find_best_location(sb, clients[fd].get_path());
                            std::string response;

                            // Check if method is allowed
                            bool method_allowed = false;
                            for (size_t i = 0; i < loc->methods.size(); ++i)
                            {
                                if (loc->methods[i] == clients[fd].get_method())
                                {
                                    method_allowed = true;
                                    break;
                                }
                            }

                            if (!method_allowed)
                                throw MethodNotAllowedException();

                            // Resolve file path
                            std::string filePath = resolve_request_path(loc->root, clients[fd].get_path(), loc->index);
                            std::string body;

                            if (filePath.empty())
                                throw BadRequestException();
                            else if (!read_file_to_string(filePath, body))
                                throw NotFoundException();
                            else
                                response = make_http_response(200, "OK", body, "text/html");

                            send(fd, response.c_str(), response.size(), 0);
                            closed = true;
                            break;
                        }
                        if (closed)
                        break;
                    } catch (const BadRequestException &e) {
                        std::cerr << RED << "[400] " << e.what() << RESET << std::endl;
                        std::string response = make_error_response_from_request(clients[fd], 400, "Bad Request");
                        send(fd, response.c_str(), response.size(), 0);
                        closed = true;
                        break;
                    } catch (const NotEmplementedException &e) {
                        std::cerr << RED << "[501] " << e.what() << RESET << std::endl;
                        std::string response = make_error_response_from_request(clients[fd], 501, "Not Implemented");
                        send(fd, response.c_str(), response.size(), 0);
                        closed = true;
                        break;
                    } catch (const MethodNotAllowedException &e) {
                        std::cerr << RED << "[405] " << e.what() << RESET << std::endl;
                        std::string response = make_error_response_from_request(clients[fd], 405, "Method Not Allowed");
                        send(fd, response.c_str(), response.size(), 0);
                        closed = true;
                        break;
                    } catch (const NotFoundException &e) {
                        std::cerr << RED << "[404] " << e.what() << RESET << std::endl;
                        std::string response = make_error_response_from_request(clients[fd], 404, "Not Found");
                        send(fd, response.c_str(), response.size(), 0);
                        closed = true;
                        break;
                    } catch (const InternalServerErrorException &e) {
                        std::cerr << RED << "[500] " << e.what() << RESET << std::endl;
                        std::string response = make_error_response_from_request(clients[fd], 500, "Internal Server Error");
                        send(fd, response.c_str(), response.size(), 0);
                        closed = true;
                        break;
                    } catch (const ForbiddenException &e) {
                        std::cerr << RED << "[403] " << e.what() << RESET << std::endl;
                        std::string response = make_error_response_from_request(clients[fd], 403, "Forbidden");
                        send(fd, response.c_str(), response.size(), 0);
                        closed = true;
                        break;
                    }
                }
                if (closed)
                {
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                    close(fd);
                    clients.erase(fd);
                }
            }
        }
    }

    close(epfd);
    close(serverFd);
    return 0;
}