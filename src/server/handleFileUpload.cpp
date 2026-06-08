#include "Server.hpp"
#include <ctime>
#include "../http/Response.hpp"

RouteInfo Server::FileUploadRoute(const RouteInfo &route, const Request &request)
{
    RouteInfo info = route;
    std::string upload_dir = route.upload_dir;
    if (upload_dir.empty() || upload_dir[upload_dir.length() - 1] != '/')
        upload_dir += "/";

    std::string filename = router->generateUploadFilename(request, route.location);
    std::string full_path = upload_dir + filename;
    std::string body = request.get_body();
    std::ofstream file(full_path.c_str(), std::ios::binary);

    if (!file.is_open())
    {
        info.action = ERROR_500;
        info.http_status = 500;
        info.status_message = "Internal Server Error";
        return info;
    }

    file.write(body.c_str(), static_cast<std::streamsize>(body.length()));
    if (file.fail())
    {
        file.close();
        info.action = ERROR_500;
        info.http_status = 500;
        info.status_message = "Internal Server Error";
        return info;
    }

    file.close();
    info.action = UPLOAD_FILE;
    info.http_status = 201;
    info.status_message = "Created";

    return info;
}
