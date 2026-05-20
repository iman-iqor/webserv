#include "Router.hpp"

RouteInfo Router::routePOST(const Request &request, Location *location)
{
    RouteInfo route_info;
    route_info.location = location;
    route_info.http_status = 200;
    route_info.status_message = "OK";

    // validate content_length
    std::string content_length_str = request.getHeader("Content-Length");
    if (!content_length_str.empty())
    {
        size_t content_length = static_cast<size_t>(atoi(content_length_str.c_str()));
        if (server_block && content_length > server_block->client_max_body_size)
        {
            route_info.action = ERROR_413;
            route_info.http_status = 413;
            route_info.status_message = "Payload Too Large";
            return route_info;
        }
    }
    std::string path = request.get_path();
    std::string file_path = resolveFilePath(path,location);

    //check for CGI scripts cause post can trigger CGI a bro
    std::string extension = getFileExtension(file_path);
    if(!location->cgi.empty() && location->cgi.count(extension) > 0)
    {
        if(fileExists(file_path) && isExecutable(file_path))
        {
            //execute cgi with post data 
            route_info.action = EXECUTE_CGI;
            route_info.cgi_string = file_path;
            route_info.http_status = 200;
            route_info.status_message = "OK";
            return route_info; 
        }
    }
    return route_info;
}