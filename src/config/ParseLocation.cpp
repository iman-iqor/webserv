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
            loc.root = expectValue();
            expectSemicolon();
        }
        else if(key == "autoindex")
        {
            std::string value = expectValue();
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
            if(i >= tokens.size() || tokens[i].type == SEMICOLON)
                throw std::runtime_error("allowed methods requires at least one method");
            while(i < tokens.size() && tokens[i].type != SEMICOLON)
            {
                validMethod(expectValue(), loc);
            }

            expectSemicolon();
        }
        else if(key == "index")
        {
            loc.index = expectValue();
            expectSemicolon();
        }
        else if(key == "upload_path")
        {
            loc.upload_path = tokens[i++].value;
            expectSemicolon();
        }
        else if(key == "cgi" || key == "cgi_pass")
        {
            std::string ext = expectValue();
            std::string path = expectValue();
            if(ext.empty() || path.empty())
                throw std::runtime_error("invalid cgi directive");
            if(loc.cgi.count(ext))
                throw std::runtime_error("duplicate cgi extension");
            loc.cgi[ext] = path;
            expectSemicolon();
        }
        else if(key == "return_url")
        {
            loc.return_url = expectValue();
            expectSemicolon();
        }
        else if(key == "return")
        {
            std::string codeStr = expectValue();
            if(!isNumber(codeStr))
                throw std::runtime_error("invalid return code");
            loc.return_code = std::atoi(codeStr.c_str());
            if(loc.return_code < 300 || loc.return_code > 399)
                throw std::runtime_error("return code must be a redirect code");
            loc.return_url = expectValue();
            expectSemicolon();
        }
        else if(key == "return_code")
        {
            std::string codeStr = expectValue();
            if(!isNumber(codeStr))
                throw std::runtime_error("invalid return code");
            loc.return_code = std::atoi(codeStr.c_str());
            if(loc.return_code < 300 || loc.return_code > 399)
                throw std::runtime_error("return code must be a redirect code");
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

void Parser::validMethod(const std::string &str, Location &loc)
{
    if(str == "GET" || str == "POST" || str == "DELETE")
    {
        for(size_t j = 0; j < loc.methods.size(); j++)
        {
            if(loc.methods[j] == str)
                throw std::runtime_error("duplicate method");
        }
        loc.methods.push_back(str);
    }
    else
        throw std::runtime_error("Unvalid method value " + str);
}