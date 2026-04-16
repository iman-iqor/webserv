#include "Parser.hpp"
#include <cstdlib>
#include <sstream>

Config Parser::parse()
{
    std::string content = readfile(_filename);
    this->tokens = tokenize(content);

    while(i < tokens.size())
    {
        if(tokens[i].value == "server")
        {
            i++;
            if(i >= tokens.size() || tokens[i].value != "{")
                throw std::runtime_error("Error: expected { after server keyword ");
            i++;
            parseServer();
        }
        else
            throw std::runtime_error("Error: undefined directive outside server block");
    }
    return _config;
}

void Parser::parseServer()
{
    ServerBlock server;
    while(i < tokens.size() && tokens[i].type != CLOSE_BRACE)
    {
        Token &token = tokens[i];
        if(token.value == "listen")
        {
            i++;
            parseListenDirective(server);
            expectSemicolon();
        }
        else if(token.value == "server_names" ||token.value == "server_name")
        {
            i++;
            while(i < tokens.size() && tokens[i].type != SEMICOLON)
            {
                server.server_names.push_back(tokens[i].value);
                i++;
            }
            expectSemicolon();
        }
        else if(token.value == "client_max_body_size")
        {
            i++;
            server.client_max_body_size = parseSize(tokens[i].value);
            i++;
            expectSemicolon();
        }
        else if(token.value == "error_pages" || token.value == "error_page")
        {
            i++;
            parseErrorPages(server);
        }
        else if(token.value == "location")
        {
            i++;
            parseLocation(server);//this too  
        }
        else if(token.value == "root")
        {
            i++;
            server.root = tokens[i].value;
            i++;
            expectSemicolon();
        }  
        else
            throw std::runtime_error("Uknown directive inside servers block");
    }
    if(i >= tokens.size() || tokens[i].type != CLOSE_BRACE)
        throw std::runtime_error("Unclosed server block");
    i++;
    _config.servers.push_back(server);
}

void Parser::expectSemicolon()
{
    if(tokens[i].type != SEMICOLON)
    {
        std::stringstream ss;
        ss << "expected semicolon at line " << tokens[i].line;
        throw std::runtime_error(ss.str());
    }
    i++;
}

static bool isValidErrorCode(std::string &str)
{
    if(str.empty())
        return false;
    for (size_t i = 0; i < str.length(); i++)
    {
        if(!isdigit(str[i]))
            return false;
    }
    int code = std::atoi(str.c_str());
    if(code < 400 || code > 599)
        throw std::runtime_error("Invalid error code, must be between 400 and 599 ");
    
    return true;;
}
void Parser::parseErrorPages(ServerBlock &server)
{
    std::vector<int> codes;
    while(i < tokens.size() && tokens[i].type == WORD && isValidErrorCode(tokens[i].value))
    {
        codes.push_back(std::atoi(tokens[i].value.c_str()));
        i++;
    }
    if(codes.empty() || i >= tokens.size() || tokens[i].type != WORD)
        throw   std::runtime_error("missing path or status code for errorPages");
    std::string path = tokens[i].value;
    i++;
    for (size_t j = 0; j < codes.size(); j++)
    {
        server.error_pages[codes[j]] = path;
    }
    expectSemicolon();
}
static bool isNumber(std::string &str)
{
    for (size_t i = 0; i < str.length(); i++)
    {
        if(!isdigit(str[i]))
            return false;
    }
    return !str.empty();
}

void Parser::parseListenDirective(ServerBlock &server)
{
    if(i >= tokens.size())
        throw std::runtime_error("missig value after listen directive");
    std::string val = tokens[i].value;
    std::string  host = "0.0.0.0";
    int port = 80;

    size_t pos = val.find(':');
    if(pos != std::string::npos)
    {
        host = val.substr(0, pos);
        std::string portStr = val.substr(pos + 1);
        if(portStr.empty() || !isNumber(portStr))
            throw std::runtime_error("mising or invalid port value after host address");
        port = std::atoi(portStr.c_str());
    }
    else
    {
        if (isNumber(val))
            port = std::atoi(val.c_str());
        else
            host = val;
    }

    if(host == "localhost")
        host = "127.0.0.1";
    
    if(port < 0 || port > 65535)
        throw std::runtime_error("port number out of range");

    server.listen_directives.push_back(std::make_pair(host, port));
    i++;
}


long Parser::parseSize(std::string &str)
{
    char last = '\0';
    if(std::isalpha(str[str.length() - 1]))
        last = std::toupper(str[str.length() - 1]);
    long multiplier = 1;

    if(last == 'K'){
        multiplier = 1024;
        str.erase(str.length() - 1);
    }
    else if(last == 'M')
    {
        multiplier = 1024 * 1024;
        str.erase(str.length() - 1);
    }
    else if(last == 'G')
    {
        multiplier = 1024 * 1024 * 1024;
        str.erase(str.length() - 1);
    }

    if(!isNumber(str) || str.empty())
        throw std::runtime_error("Invalid number for Client_max_body_size");
    long size = std::atol(str.c_str()) * multiplier; 
    if(size > 1073741824)
        throw std::runtime_error("Client_max_body_size exceeds 1G limit");
    return  size;
}


void Parser::parseLocation(ServerBlock &server)
{
    if(i >= tokens.size() || tokens[i].type != WORD)
        throw std::runtime_error("expecting path after location keyword");
    
    Location loc;
    loc.root = server.root;
    loc.path = tokens[i].value;
    i++;
    if(i >= tokens.size() || tokens[i].type != OPEN_BRACE)
        throw std::runtime_error("expected '{ ' for location block");
    i++;
    while(i < tokens.size() && tokens[i].type != CLOSE_BRACE)
    {
        std::string key = tokens[i++].value;
        if(key == "root")
        {
            loc.root = tokens[i++].value;
            expectSemicolon();
        }
        else if(key == "autoindex")
        {
            loc.autoindex = (tokens[i++].value == "on");
            expectSemicolon();
        }
        else if(key == "methods" || key == "allowed_methods")
        {
            while(i < tokens.size() && tokens[i].type != SEMICOLON)
            {
                loc.methods.push_back(tokens[i].value);
                i++;
            }
            expectSemicolon();
        }
        else if(key == "index")
        {
            loc.index = tokens[i++].value;
            expectSemicolon();
        }
        else if(key == "upload_path")
        {
            loc.upload_path = tokens[i++].value;
            expectSemicolon();
        }
        else if(key == "cgi" || key == "cgi_pass")
        {
            std::string ext = tokens[i++].value;
            std::string path = tokens[i++].value;
            loc.cgi[ext] = path;
            expectSemicolon();
        }
        else if(key == "return_url" || key == "return")
        {
            if(key == "return")
            {
                loc.return_code = std::atoi(tokens[i++].value.c_str());
                loc.return_url = tokens[i++].value;
            }
            else
            {
                loc.return_url = tokens[i++].value;
            }
            expectSemicolon();
        }
        else if(key == "return_code")
        {
            loc.return_code = std::atoi(tokens[i++].value.c_str());
            expectSemicolon();
        }
        else
            throw std::runtime_error("unvalid directive used inside location block");
    }

    if(i >= tokens.size() || tokens[i].type != CLOSE_BRACE)
        throw std::runtime_error("expected '}' for location block");
    i++;

    server.locations.push_back(loc);
}
Parser::Parser(std::string filename)
{
    this->i = 0;
    this->_filename = filename;
}