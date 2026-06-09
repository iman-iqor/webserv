#include "Router.hpp"
#include <ctime>
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
        if (server_block && static_cast<long>(content_length) > server_block->client_max_body_size)
        {
            route_info.action = ERROR_413;
            route_info.http_status = 413;
            route_info.status_message = "Payload Too Large";
            return route_info;
        }
    }
    std::string path = request.get_path();
    std::string file_path = resolveFilePath(path, location);

    // check for CGI scripts cause post can trigger CGI a bro
    std::string extension = getFileExtension(file_path);
    std::cout << "extension: " << extension << std::endl;

    if (!location->cgi.empty())
    {
        std::cout << "inside cgi" << std::endl;
        if (fileExists(file_path) && isExecutable(file_path))
        {
            // execute cgi with post data
            route_info.file_extension = extension;
            route_info.action = EXECUTE_CGI;
            route_info.cgi_string = file_path;
            route_info.http_status = 200;
            route_info.status_message = "OK";
            return route_info;
        }
    }

    // check if uploads are allowed
    // router only decides if upload is alloowed
    // the actual upload happens in server::processRequest
    if (isUploadAllowed(location))
    {
        route_info.action = UPLOAD_FILE;
        route_info.upload_dir = location->upload_path;
        route_info.http_status = 201; // created
        route_info.status_message = "created";
        return route_info;
    }

    // post ot alloed
    route_info.action = ERROR_405;
    route_info.http_status = 405;
    route_info.status_message = "Method Not Allowed";

    return route_info;
}

std::string Router::generateUploadFilename(const Request &request, Location *location)
{
    (void)location;

    // try to get filename from content-disposition

    try
    {
        std::string disposition = request.getHeader("Content-Disposition");
        if (!disposition.empty())
        {
            size_t filename_pos = disposition.find("filename=\"");

            if (filename_pos != std::string::npos)
            {
                size_t start = filename_pos + 10;
                size_t end = disposition.find("\"", start);
                if (end != std::string::npos)
                {
                    std::string filename = disposition.substr(start, end - start);

                    // some security:remove path traversal attempts
                    // remove "/" from file\name
                    size_t slash_pos = filename.rfind('/');
                    if (slash_pos != std::string::npos)
                        filename = filename.substr(slash_pos + 1);

                    // check for ".." path traversal
                    if (filename.find("..") != std::string::npos)
                        return ""; // reject malicious filename

                    return filename;
                }
            }
        }
    }
    catch (const BadRequestException &e)
    {
        // if the header is malformed i will just ignore it and generate a default filename
        (void)e;
    }

    return "upload_" + timeToString(std::time(NULL));
}