#include"Router.hpp"
RouteInfo Router::routeGET(const Request &request,Location* location)
    {
        RouteInfo route_info;
        route_info.location = location;

        std::string file_path = resolveFilePath(request.get_path(),location);
        std::cout<<"resolved file path: "<<file_path<<std::endl;
        if(!fileExists(file_path) && location->autoindex == false)
        {

            std::cout<<"no file no dir"<<std::endl;
            route_info.action = ERROR_404;
            route_info.http_status = 404;
            route_info.status_message = "Not Found";
            return route_info;
        }

        if(isDirectory(file_path) || location->autoindex == true)
        {
            if(location->autoindex)
            {
                std::cout<<"\033[31mautoindex enabled\033[31m"<<std::endl;
                route_info.action = DIRECTORY_LISTING;
                route_info.file_path = file_path;
                route_info.http_status  = 200;
                route_info.status_message = "OK";
                return route_info;
            }
            std::cout<<"is a directory"<<std::endl;
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


            route_info.action = ERROR_403;
            route_info.http_status = 403;
            route_info.status_message = "Forbidden";
            return route_info;
        }

        std::string extension = getFileExtension(file_path);
        if(!location->cgi.empty() && location->cgi.count(extension) > 0)
        {
            if(isExecutable(file_path))
            {
                route_info.action = EXECUTE_CGI;
                route_info.cgi_string = file_path;
                route_info.http_status = 200;
                route_info.status_message = "OK";
                return route_info;
            }
        }
        std::cout<<"\033[31mserving file\033[31m"<<std::endl;
        route_info.action = SERVE_FILE;
        route_info.file_path = file_path;
        route_info.http_status = 200;
        route_info.status_message = "OK";

        return route_info;  
    }