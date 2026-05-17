

#include "Router.hpp"
#include <sys/stat.h>//for stat function to check if file exists and if it's a directory
#include <dirent.h>//for opendir and readdir to list directory content

Router::Router(Config *config)
{
    this->config = config;
    this->server_block = NULL;
}

RouteInfo Router::route(const Request &request,ServerBlock* server_block)
{
    this->server_block = server_block;
    RouteInfo route_info;
    route_info.location = NULL;
    route_info.headers.clear();

    route_info.location = findMatchingLocation(request.get_path());
    if(!route_info.location)//what should i do if i did not fin d thelocation ?
    {
        route_info.action = ERROR_404;
        route_info.http_status = 404;
        route_info.status_message = "Not Found";
        return route_info;
    }

    if(!isMethodAllowed(request.get_method(),route_info.location))
    {
        




        
    }

    return route_info;
}