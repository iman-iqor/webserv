#include "Router.hpp"
RouteInfo Router::routeGET(const Request &request, Location *location)
{
    RouteInfo route_info;
    route_info.location = location;

    std::string file_path = resolveFilePath(request.get_path(), location);
    std::cout << "resolved file path: " << file_path << std::endl;
    if (!fileExists(file_path))
    {
        std::cout << "\033[31mfile does not exist\033[31m" << std::endl;
        route_info.action = ERROR_404;
        route_info.http_status = 404;
        route_info.status_message = "Not Found";
        return route_info;
    }

    if (isDirectory(file_path))
    {
        std::string index_path = file_path;
        if (index_path[index_path.size() - 1] != '/')
            index_path += "/";
        index_path += location->index;
        if (fileExists(index_path) && !isDirectory(index_path))
        {
            std::cout << "\033[31mindex file found\033[31m" << std::endl;
            route_info.action = SERVE_FILE;
            route_info.file_path = index_path;
            route_info.http_status = 200;
            route_info.status_message = "OK";
            return route_info;
        }

        if (location->autoindex == true)
        {
            std::cout << "\033[31mautoindex enabled\033[31m" << std::endl;
            route_info.action = DIRECTORY_LISTING;
            route_info.file_path = file_path;
            route_info.http_status = 200;
            route_info.status_message = "OK";
            return route_info;
        }

        route_info.action = ERROR_403;
        route_info.http_status = 403;
        route_info.status_message = "Forbidden";
        return route_info;
    }

    std::string extension = getFileExtension(file_path);
    if (!location->cgi.empty())
    {
        std::cout << "\033[31mfound CGI handler for extension " << extension << "\033[31m" << std::endl;
        if (isExecutable(file_path))
        {
            route_info.action = EXECUTE_CGI;
            route_info.cgi_string = file_path;
            route_info.http_status = 200;
            route_info.status_message = "OK";
            return route_info;
        }
    }
    std::cout << "\033[31mserving file\033[31m" << std::endl;
    route_info.action = SERVE_FILE;
    route_info.file_path = file_path;
    route_info.http_status = 200;
    route_info.status_message = "OK";

    return route_info;
}