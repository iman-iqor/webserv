#include "Config.hpp"

#include <sstream>
#include <fstream>

std::string readfile(std::string filename)
{
    std::ifstream file(filename.c_str());
    if(!file.is_open())
        throw std::runtime_error("can not open file");

    std::stringstream buff;
    buff << file.rdbuf();
    file.close();
    return buff.str();
}

void setType(Token &token)
{
    if(token.value == "{")
        token.type = OPEN_BRACE;
    else if(token.value == "}")
        token.type = CLOSE_BRACE;
    else if(token.value == ";")
        token.type = SEMICOLON;
    else
        token.type = WORD;
}

std::vector<Token> tokenize(std::string &content)
{
    std::string newContent;
    std::vector<Token> tokens;
    int current_line = 1;

    for (size_t i = 0; i < content.length(); i++)
    {
        if(content[i] == '\n')
        {
            current_line++;
            continue;
        }
        if(content[i] == '#')
        {
            while(i < content.length() && content[i] != '\n')
                i++;
            i--;
            continue;
        }
        if(isspace(content[i]))
            continue;

        if(content[i] == '{' || content[i] == '}' || content[i] == ';')
        {
            Token newToken;
            newToken.value = std::string(1, content[i]);
            newToken.line = current_line;
            setType(newToken);
            tokens.push_back(newToken);
            continue;
        }
        
        std::string word;
        while (i < content.length() && !isspace(content[i]) && content[i] != '{'
                && content[i] != '}' && content[i] != ';')
        {
            word += content[i];
            i++;
        }
        if(!word.empty())
        {
            Token t;
            t.value = word;
            t.line = current_line;
            setType(t);
            tokens.push_back(t);
        }
        i--;
    } 
    return tokens;
}