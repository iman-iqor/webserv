#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <iostream>
#include <vector>
#include <map>

enum token_type{
    WORD,
    OPEN_BRACE,
    CLOSE_BRACE,
    SEMICOLON
};

struct Token {
    std::string value;
    int         type;
    int         line;
};

struct Location {
    std::string path;
    std::string root;
    std::vector<std::string> methods;
    std::string index;
    bool autoindex;
    std::map<std::string, std::string> cgi; // this for cgi to support multiples languages
    std::string upload_path;
    std::string return_url;
    int return_code;

    Location() : autoindex(false), return_code(0) {};
};

struct ServerBlock {
    std::vector<std::pair<std::string, int> > listen_directives;
    std::vector<std::string> server_names;
    long client_max_body_size;
    std::map<int, std::string> error_pages;
    std::vector<Location> locations;
    std::string root;

    ServerBlock() : client_max_body_size(1048576) {};
};

struct Config {
    std::vector<ServerBlock> servers;
};


std::string readfile(std::string filename);
std::vector<Token> tokenize(std::string &content);

#endif