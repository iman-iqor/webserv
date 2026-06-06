#include "Parser.hpp"

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
            server.client_max_body_size = parseSize(expectValue());
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
            parseLocation(server);
        }
        else if(token.value == "root")
        {
            i++;
            server.root = expectValue();
            expectSemicolon();
        }  
        else
            throw std::runtime_error("Uknown directive inside servers block");
    }
    if(i >= tokens.size() || tokens[i].type != CLOSE_BRACE)
        throw std::runtime_error("Unclosed server block");
    i++;

    if(server.listen_directives.empty())
        server.listen_directives.push_back(std::make_pair("0.0.0.0", 8080));
    
    _config.servers.push_back(server);
}



Parser::Parser(std::string filename)
{
    this->i = 0;
    this->_filename = filename;
}


