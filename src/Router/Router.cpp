

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
    else if(method == "POST")
        return routePOST(request,route_info.location);
    // else if(method  == "DELETE")
    //     return routeDELETE(request,route_info.location);
    
    route_info.action = ERROR_405;
    route_info.http_status = 405;
    route_info.status_message = "Method Not Allowed";

    return route_info;
}

    

