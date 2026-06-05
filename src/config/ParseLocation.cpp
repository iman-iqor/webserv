#include "Parser.hpp"

void Parser::parseLocation(ServerBlock &server)
{
    if(i >= tokens.size() || tokens[i].type != WORD)
        throw std::runtime_error("expecting path after location keyword");
    
    Location loc;
    loc.root = server.root;
    loc.path = expectValue();
    if (loc.path.empty())
        throw std::runtime_error("empty location path");
    loc.return_url = "";
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
            std::string value = tokens[i++].value;
            if(value == "on")
                loc.autoindex = true;
            else if(value == "off")
                loc.autoindex = false;
            else
                throw std::runtime_error("autoindex must be on or off");
            expectSemicolon();
        }
        else if(key == "methods" || key == "allowed_methods")
        {
            while(i < tokens.size() && tokens[i].type != SEMICOLON)
            {
               validMethod(tokens[i].value, loc);
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

void Parser::validMethod(std::string &str, Location &loc)
{
    if(str == "GET" || str == "POST" || str == "DELETE")
    {
        loc.methods.push_back(str);
    }
    else
        throw std::runtime_error("Unvalid method value" + str);
}