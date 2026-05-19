

#include "Router.hpp"

#include<string>

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
        route_info.action = ERROR_405;
        route_info.http_status = 405;
        route_info.status_message = "Method Not Allowed";
        return route_info;
    }

    if(!route_info.location->return_url.empty())
    {
        route_info.action = REDIRECT;
        route_info.http_status = route_info.location->return_code;
        route_info.redirect_url = route_info.location->return_url;
        route_info.status_message = "Moved Prmanently";
        return route_info;
    }

    std::string method = request.get_method();

    if(method == "GET")
        return routeGET(request,route_info.location);
    // else if(method == "POST")
    //     return routePOST(request,route_info.location);
    // else if(method  == "DELETE")
    //     return routeDELETE(request,route_info.location);
    
    route_info.action = ERROR_405;
    route_info.http_status = 405;
    route_info.status_message = "Method Not Allowed";

    return route_info;
}

    RouteInfo Router::routeGET(const Request &request,Location* location)
    {
        RouteInfo route_info;
        route_info.location = location;

        std::string file_path = resolveFilePath(request.get_path(),location);
        
        std::cout<<"\033[31mhello\033[31m"<<std::endl;
        if(!fileExists(file_path))
        {
            route_info.action = ERROR_404;
            route_info.http_status = 404;
            route_info.status_message = "Not Found";
            return route_info;
        }

        if(isDirectory(file_path))
        {
            std::string index_path = file_path;
            if(file_path[file_path.size()-1] != '/')
                index_path += "/";
            
            index_path += location->index;

            if(fileExists(index_path) && !isDirectory(index_path))
            {
                std::cout<<"\033[31mindex file found\033[31m"<<std::endl;
                route_info.action = SERVE_FILE;
                route_info.file_path = index_path;
                route_info.http_status = 200;
                route_info.status_message = "OK";
                return route_info;
            }

            if(location->autoindex)
            {
                std::cout<<"\033[31mautoindex enabled\033[31m"<<std::endl;
                route_info.action = DIRECTORY_LISTING;
                route_info.file_path = file_path;
                route_info.http_status  = 200;
                route_info.status_message = "OK";
                return route_info;
            }

            route_info.action = ERROR_403;
            route_info.http_status = 403;
            route_info.status_message = "Forbidden";
            return route_info;
        }
        return route_info;  
    }

