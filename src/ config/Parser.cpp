#include "parser.hpp"
#include <cstdlib>
Config Parser::parse()
{
    while(i < tokens.size())
    {
        if(tokens[i].value == "server")
        {
            i++;
            if(i >= tokens.size() || tokens[i].value != "{")
                throw std::runtime_error("Error: expected { after server keyword in line: " + tokens[i].line);
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
            parseListenDirective(server);// i will implement this next
            expectSemicolon();
        }
        else if(token.value == "server_names")
        {
            //add loop here for multiple server names
            i++;
            server.server_names.push_back(tokens[i].value);
            i++;
            expectSemicolon();
        }
        else if(token.value == "client_max_body_size")
        {
            i++;
            server.client_max_body_size = parseSize(tokens[i].value);
            i++;
            expectSemicolon();
        }
        else if(token.value == "error_pages")
        {
            parseErrorPages(server);
        }
        else if(token.value == "location")
        {
            parseLocation(server);//this too 
        }
        else if(token.value == "root")
        {
            i++;
            server.root = tokens[i].value;
            expectSemicolon();
        }  
    }
    if(i >= tokens.size() || tokens[i].type != CLOSE_BRACE)
        throw std::runtime_error("Unclosed server block");
    i++;
    _config.servers.push_back(server);
}

void Parser::expectSemicolon()
{
    if(tokens[i].type != SEMICOLON)
        throw std::runtime_error("expected semicolon at line " + tokens[i].line);
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
        throw   std::runtime_error("missing path or status code for errorPages at line");
    std::string path = tokens[i].value;
    i++;
    for (size_t j = 0; j < codes.size(); j++)
    {
        server.error_pages[codes[j]] = path;
    }
    expectSemicolon();
}

void Parser::parseListenDirective(ServerBlock &server)
{

}

static bool isNumber(std::string &str)
{
    for (size_t i = 0; i < str.length(); i++)
    {
        if(!isdigit(str[i]))
            return false;
    }
    return str.empty();
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