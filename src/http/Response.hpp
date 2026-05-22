#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "Router.hpp"
#include <map>
#include <string>
#include <sstream>
#include <dirent.h>



typedef struct CgiResponse_s
{
    std::map<std::string, std::string> headers;
    std::string body;
    int status_code;
    std::string status_message;
} CgiResponse_t;


class Response
{
    private:
        std::string http_version;
        int status_code;
        std::string status_message;
        std::map<std::string, std::string> headers;
        std::string body;

    public:
        Response();
        ~Response();
        void setStatus(int code, const std::string &statusMsg);
        void setHeader(const std::string &key, const std::string &value);
        void setBody(const std::string &body);
        void setContentLength();
        std::string build();
        void handleResponse(const RouteInfo &info, CgiResponse_t& cgi_output, const std::map<int, std::string > error_pages);
        void handleAutoIndex(const RouteInfo &info);
        void serveFile(const std::string &file_path, const std::map<int, std::string > error_pages);
        void ErrorResponse(int code, const std::string &msg, const std::map<int, std::string > error_pages);
        std::string MimeType(const std::string &path);
        void handleRedirect(const RouteInfo &info);
        void handleCGI(CgiResponse_t& cgi_output, const std::map<int, std::string > error_pages);
        
};

#endif