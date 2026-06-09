#include "Server.hpp"

RouteInfo Server::DeleteFile(const RouteInfo &route)
{
    RouteInfo info = route;

    if (unlink(route.file_path.c_str()) != 0)
    {
        if (errno == EACCES)
        {
            info.action = ERROR_403;
            info.http_status = 403;
            info.status_message = "Forbidden";
        }
        else if (errno == ENOENT)
        {
            info.action = ERROR_404;
            info.http_status = 404;
            info.status_message = "Not Found";
        }
        else
        {
            info.action = ERROR_500;
            info.http_status = 500;
            info.status_message = "Internal Server Error";
        }
        return info;
    }
    info.action = ERROR_200;
    info.http_status = 204;
    info.status_message = "No Content";

    return info;
}