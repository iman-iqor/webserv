

#include "Router.hpp"

#include <string>

Router::Router(Config *config)
{
    this->config = config;
    this->server_block = NULL;
}

std::string Router::resolveErrorPage(int error_code)
{
    if(!this->server_block)
        return "";
    
    std::map<int,std::string>:: iterator it = this->server_block->error_pages.find(error_code);

    if(it == this->server_block->error_pages.end())
        return "";

    std::string page_path = it->second;

    //find the x in the path and replace it with the last digit of code
    size_t x_pos = page_path.rfind('x');
    if(x_pos != std::string::npos)
    {
        page_path[x_pos]='0' + (error_code % 10);
    }

    std::string full_path = this->server_block->root + page_path;
    if (fileExists(full_path))
        return full_path;
    return "";
}

RouteInfo Router::route(const Request &request, ServerBlock *server_block)
{
    this->server_block = server_block;
    RouteInfo route_info;
    route_info.location = NULL;
    route_info.headers.clear();

    route_info.location = findMatchingLocation(request.get_path());
    if (!route_info.location)
    {
        route_info.http_status = 404;
        route_info.status_message = "Not Found";
        
        std::string error_page=resolveErrorPage(404);
        if (!error_page.empty())
        {
            route_info.action = SERVE_FILE;
            route_info.file_path = error_page;
        }
        else
            route_info.action = ERROR_404;
        
        
        return route_info;
    }

    if (!isMethodAllowed(request.get_method(), route_info.location))
    {
        route_info.http_status = 405;
        route_info.status_message = "Method Not Allowed";
        std::string error_page=resolveErrorPage(405);

        if (!error_page.empty())
        {
            route_info.action = SERVE_FILE;
            route_info.file_path = error_page;
        }
        else
            route_info.action = ERROR_405;
        return route_info;
    }

    if (!route_info.location->return_url.empty())
    {
        route_info.action = REDIRECT;
        route_info.http_status = route_info.location->return_code;
        route_info.redirect_url = route_info.location->return_url;
        route_info.status_message = "Moved Prmanently";
        return route_info;
    }

    std::string method = request.get_method();

    if (method == "GET")
        return routeGET(request, route_info.location);
    else if (method == "POST")
        return routePOST(request, route_info.location);
    else if (method == "DELETE")
        return routeDELETE(request, route_info.location);

    route_info.http_status = 405;
    route_info.status_message = "Method Not Allowed";
     std::string error_page=resolveErrorPage(405);
    if (!error_page.empty())    {
        route_info.action = SERVE_FILE;
        route_info.file_path = error_page;
    }
    else
        route_info.action = ERROR_405;

    return route_info;
}
