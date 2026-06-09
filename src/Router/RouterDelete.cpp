#include "Router.hpp"

RouteInfo Router::routeDELETE(const Request &request, Location *location)
{
    RouteInfo route_info;
    route_info.location = location;

    std::string file_path = resolveFilePath(request.get_path(), location);
    if (!fileExists(file_path))
    {
        route_info.http_status = 404;
        route_info.status_message = "Not Found";
        std::string error_page=resolveErrorPage(404,server_block);
        if (!error_page.empty())        {
            route_info.action = SERVE_FILE;
            route_info.file_path = error_page;
        }
        else
            route_info.action = ERROR_404;
        return route_info;
    }

    if (isDirectory(file_path))
    {
        route_info.http_status = 403;
        route_info.status_message = "Forbidden";
        std::string error_page=resolveErrorPage(403,server_block);
        if (!error_page.empty())
        {
            route_info.action = SERVE_FILE;
            route_info.file_path = error_page;
        }
        else
            route_info.action = ERROR_403;
        return route_info;
    }
    // file can be deleted
    route_info.action = DELETE_FILE;
    route_info.file_path = file_path;
    route_info.http_status = 204; // no content
    route_info.status_message = "No Content";
    return route_info;
}