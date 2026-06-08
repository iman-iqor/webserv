#include "Response.hpp"

Response::Response(Server &serv) : http_version("HTTP/1.1"), status_code(0), server(serv)
{   
}


void Response::setBody(const std::string& bodyValue)
{
    this->body = bodyValue;
    this->setContentLength();
}

void Response::setStatus(int code, const std::string& statusMsg)
{
    status_message = statusMsg;
    status_code = code;
}

void Response::setHeader(const std::string& key, const std::string& value)
{
    headers[key] = value;
}

static std::string intToString(int len)
{
    std::stringstream ss;
    ss << len;
    return ss.str();
}

void Response::setContentLength()
{
    this->setHeader("Content-Length", intToString(body.length()));
}

Response::~Response()
{
}

std::string Response::build()
{
    std::stringstream ss;

    ss << http_version << " " << intToString(status_code) << " " << status_message << "\r\n";

    std::map<std::string, std::string>::iterator it;
    for (it = headers.begin(); it != headers.end(); it++)
    {
        ss << it->first << ": " << it->second << "\r\n";
    }
    ss << "\r\n";
    ss << body;
    return ss.str();
}

// void Response::handleResponse(int client_fd, const RouteInfo& info, CgiResponse_t& cgi_output, const std::map<int, std::string> error_pages, Client &client)

void Response::handleResponse(const RouteInfo& info, const std::map<int, std::string> error_pages, Client *client)
{
    this->setStatus(info.http_status,info.status_message);

    std::map<std::string, std::string>::const_iterator it;
    for (it = info.headers.begin(); it != info.headers.end(); it++)
    {
        this->setHeader(it->first, it->second);
    }

    if(info.http_status >= 400 and info.action != SERVE_FILE)
    {
        this->ErrorResponse(info.http_status, info.status_message, error_pages);
        return;
    }
    switch (info.action)
    {
        case SERVE_FILE:
            this->serveFile(info.file_path, error_pages);
            break;
        case REDIRECT:
            handleRedirect(info);
            break;
        case DIRECTORY_LISTING:
            handleAutoIndex(info);
            break;
        case UPLOAD_FILE:
            handleFileUpload(info, client->request, error_pages);
            break;
        case DELETE_FILE:
            handleFileDelete( info, error_pages);
            break;
        default:
            break;
    }
    
}

void Response::serveFile(const std::string& file_path, const std::map<int, std::string> error_pages)
{
    std::ifstream file(file_path.c_str(), std::ios::in | std::ios::binary);
    if(!file.is_open())
    {
        this->ErrorResponse(404, "Not Found" ,error_pages);
        return;
    }

    std::stringstream ss;
    ss << file.rdbuf();
    file.close();
    this->setStatus(200, "OK");
    this->setHeader("Content-Type", MimeType(file_path));
    this->setBody(ss.str());
}


std::string Response::MimeType(const std::string& path)
{
    std::map<std::string, std::string> m;
    m[".html"] = "text/html";
    m[".htm"]  = "text/html";
    m[".css"]  = "text/css";
    m[".js"]   = "application/javascript";
    m[".png"]  = "image/png";
    m[".jpg"]  = "image/jpeg";
    m[".jpeg"] = "image/jpeg";
    m[".gif"]  = "image/gif";
    m[".txt"]  = "text/plain";
    m[".zip"]  = "application/zip";
    m[".pdf"]  = "application/pdf";

    size_t pos = path.find_last_of('.');
    if(pos == std::string::npos)
        return "application/octet-stream";
    std::string ext = path.substr(pos);
    if(m.find(ext) != m.end())
        return m[ext];
    return "application/octet-stream";
}


void Response::ErrorResponse(int code, const std::string &msg, const std::map<int, std::string > error_pages)
{
    std::string error_body;
    const std::map<int, std::string>::const_iterator it = error_pages.find(code);
    if(it != error_pages.end())
    {
        std::ifstream file(it->second.c_str(), std::ios::in | std::ios::binary);
        if(file.is_open())
        {
            std::stringstream ss;
            ss << file.rdbuf();
            error_body = ss.str();
            file.close();
        }
    }
    
    if(error_body.empty())
    {
        std::stringstream ss;
        ss << "<html><head><title>" << code << " " << msg << "</title></head>";
        ss << "<body><center><h1>" << code << " " << msg << "</h1></center></body></html>";
        error_body= ss.str();    
    }

    this->setStatus(code, msg);
    this->setHeader("Content-Type", "text/html");
    this->setBody(error_body);
}

void Response::handleAutoIndex(const RouteInfo &info)
{
    DIR* dir = opendir(info.file_path.c_str());
    if(!dir)
        return;
    
    std::stringstream ss;
    ss << "<html><head><title>" << info.location->path << "</title></head>\n";
    ss << "<body><h1>" << info.location->path << "</h1><hr><pre>\n";

    std::string base_url = info.location->path;
    if(!base_url.empty() && base_url[base_url.length() - 1] != '/')
        base_url += "/";
    
    struct dirent* entry;
    while((entry = readdir(dir)) != NULL)
    {
        std::string name = entry->d_name;
        if(name == ".")
            continue;
        ss << "<a href=\"" << base_url << name << "\">" << name << "</a>\n";
    }

    ss << "</pre><hr></body></html>\n";
    closedir(dir);
    this->setStatus(200, "OK");
    this->setHeader("Content-Type", "text/html");
    this->setBody(ss.str());
}

void Response::handleRedirect(const RouteInfo &info)
{
    this->setStatus(info.http_status, info.status_message);
    this->setHeader("Location", info.redirect_url);
    this->setBody("");
}


void Response::handleCGIres(CgiResponse_t& cgi_output)
{
    std::map<std::string , std::string>::iterator it;
    for (it = cgi_output.headers.begin(); it != cgi_output.headers.end(); it++)
    {
        this->setHeader(it->first, it->second);

    }
    if(cgi_output.headers.find("Content-Type") == cgi_output.headers.end())
    {
        this->setHeader("Content-Type", "text/plain");
    }
    this->setBody(cgi_output.body);
    this->setStatus(cgi_output.status_code, cgi_output.status_message);

}

void Response::handleFileUpload(const RouteInfo &info, Request &req, const std::map<int, std::string > error_pages)
{
    RouteInfo upload_info = server.FileUploadRoute(info, req);

    this->setStatus(upload_info.http_status,upload_info.status_message);

    if (upload_info.http_status >= 400)
    {
        ErrorResponse(upload_info.http_status, upload_info.status_message, error_pages);
    }
    else
    {
        setHeader("Content-Type", "text/plain");
        setBody("File uploaded successfully");
    }
}

void Response::handleFileDelete(const RouteInfo &info, const std::map<int, std::string > error_pages)
{
    RouteInfo upload_info = server.DeleteFile(info);

    this->setStatus(upload_info.http_status,upload_info.status_message);

    if (upload_info.http_status >= 400)
    {
        ErrorResponse(upload_info.http_status, upload_info.status_message, error_pages);
    }
    else
    {
        setStatus(204, "No Content");
        setBody("");
    }
}