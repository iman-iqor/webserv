#include "Router.hpp"
RouteInfo Router::routeGET(const Request &request, Location *location)
{
    RouteInfo route_info;
    route_info.location = location;
    if(request.get_method()=="HEAD")
        route_info.isHead=true;
    std::string file_path = resolveFilePath(request.get_path(), location);
    if (!fileExists(file_path)&& !isDirectory(file_path))
    {
        route_info.http_status = 404;
        route_info.status_message = "Not Found";
        std::string error_page=resolveErrorPage(404,server_block);
        if (!error_page.empty())
        {
            std::cout<<"olala"<<std::endl;
            route_info.action = SERVE_FILE;
            route_info.file_path = error_page;
        }
        else
            route_info.action = ERROR_404;
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
            route_info.action = SERVE_FILE;
            route_info.file_path = index_path;
            route_info.http_status = 200;
            route_info.status_message = "OK";
            return route_info;
        }

        if (location->autoindex == true)
        {
            route_info.action = DIRECTORY_LISTING;
            route_info.file_path = file_path;
            route_info.http_status = 200;
            route_info.status_message = "OK";
            return route_info;
        }

        route_info.http_status = 404;
        route_info.status_message = "Not Found";
        std::string error_page=resolveErrorPage(404,server_block);
        if (!error_page.empty())
        {
            route_info.action = SERVE_FILE;
            route_info.file_path = error_page;
        }
        else
            route_info.action = ERROR_403;
        return route_info;
    }

    std::string extension = getFileExtension(file_path);
    if (!location->cgi.empty())
    {
       
        if (isExecutable(file_path))
        {
            route_info.file_extension = extension;
            route_info.action = EXECUTE_CGI;
            route_info.cgi_string = file_path;
            route_info.http_status = 200;
            route_info.status_message = "OK";
            return route_info;
        }
    }
    route_info.action = SERVE_FILE;
    route_info.file_path = file_path;
    route_info.http_status = 200;
    route_info.status_message = "OK";

    return route_info;
}