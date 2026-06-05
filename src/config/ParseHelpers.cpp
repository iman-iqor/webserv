#include "Parser.hpp"

void Parser::expectSemicolon()
{
    if(i >= tokens.size())
        throw std::runtime_error("unexpected end of file");
    if(tokens[i].type != SEMICOLON)
    {
        std::stringstream ss;
        ss << "expected semicolon at line " << tokens[i].line;//line counting  is not accurate for now
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
    if(i >= tokens.size() || tokens[i].type != WORD)
        throw std::runtime_error("missig value after listen directive");
    std::string val = tokens[i].value;
    std::string  host = "0.0.0.0";
    int port = 80;

    size_t pos = val.find(':');
    if(pos != std::string::npos)
    {
        host = val.substr(0, pos);
        std::string portStr = val.substr(pos + 1);
        if (portStr.empty() || !isNumber(portStr))
            throw std::runtime_error("missing or invalid port");
        long portLong = std::atol(portStr.c_str());
        if (portLong < 0 || portLong > 65535)
            throw std::runtime_error("port out of range");
        port = static_cast<int>(portLong);
    }
    else
    {
        if (isNumber(val))
        {
            long portLong = std::atol(val.c_str());
            if (portLong < 0 || portLong > 65535)
                throw std::runtime_error("port out of range");
            port = static_cast<int>(portLong);
        }
        else
            host = val;
    }
    if(host == "localhost")
        host = "127.0.0.1";
    if (!isValidIPv4(host))
        throw std::runtime_error("invalid host address");

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

std::string Parser::expectValue()
{
    if(i >= tokens.size())
        throw std::runtime_error("unexpected end of file");

    if(tokens[i].type != WORD)
    {
        std::stringstream ss;
        ss << "expected value at line " << tokens[i].line;
        throw std::runtime_error(ss.str());
    }

    return tokens[i++].value;
}

bool Parser::isValidIPv4(const std::string &ip)
{
    std::stringstream ss(ip);
    std::string part;
    int count = 0;

    while (std::getline(ss, part, '.'))
    {
        if (part.empty())
            return false;

        for (size_t i = 0; i < part.size(); i++)
        {
            if (!isdigit(part[i]))
                return false;
        }

        int num = std::atoi(part.c_str());

        if (num < 0 || num > 255)
            return false;

        count++;
    }

    return count == 4;
}