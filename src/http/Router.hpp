//imane 's file

#ifndef ROUTER_HPP
#define ROUTER_HPP

#include"../server/Client.hpp"

enum RouteAction
{
    SERVE_FILE, //return a static file file 3adi i think this when auto index is off
    DIRECTORY_LISTING,//list the directory conntent and this when the auto index is on
    REDIRECT,//if the clients asked me about an old file i will send him a new path to visit with 301-302 redirect
    EXECUTE_CGI,//run scripts u sf + the methos could be post or get normal
    ERROR_404,
    ERROR_403,
    ERROR_405,
    ERROR_400,
    ERROR_413,
    ERROR_301,
    ERROR_302,
    ERROR_500,
    ERROR_200,
    ERROR_411,

};

struct RouteInfo
{
    RouteAction action;//what to do
    std::string file_path;//path of the file to server file actio
    std::string redirect_url;
    std::string cgi_string;//script path for execute cgi
    Location *location;//the location block that matched the request
    int http_status;
    std::string status_message;//ok or not found li  7bit
    std::map<std::string,std::string> headers;//extra headers n9darnhtajom

};

class Router
{
    private:
        Config *config;
        ServerBlock* server_block;

        //these are just my helper methods a sahbti oumaima ..bisous
        Location* findMatchingLocation(const std::string &path);
        bool isMethodAllowed(const std::string &method,Location* location);
        std::string resolveFilePath(const std::string &path,Location* location);
        bool isDirectoryListingAllowed(Location* location);
        bool fileExists(const std::string &path);
        bool isDirectory(const std::string &path);
        bool isExecutable(const std::string &path);
        std::string getFileExtension(const std::string &path);
    public:
        Router(Config *config);
        RouteInfo route(const Request &request,ServerBlock* server_block);

        RouteInfo roteGET(const Request &request,Location* location);
        RouteInfo rotePOSTE(const Request &request,Location* location);
        RouteInfo roteDELETE(const Request &request,Location* location);
};

#endif