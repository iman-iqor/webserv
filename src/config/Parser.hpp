#ifndef PARSER_HPP
#define PARSER_HPP

#include "Config.hpp"
#include <cstdlib>
#include <sstream>

class Parser
{
    private:
        std::vector<Token> tokens;
        size_t i;
        Config _config;
        std::string _filename;
        
        void parseServer();
        void parseLocation(ServerBlock &server);
        void parseDirective(ServerBlock &server);
        void expectSemicolon();
        std::string expectValue();
        void parseListenDirective(ServerBlock &server);
        long parseSize(const std::string &str);
        void parseErrorPages(ServerBlock &server);
        void validMethod(const std::string &str, Location &loc);
        bool isValidIPv4(const std::string &ip);
        bool isNumber(std::string &str);
    public:
        Parser(std::string filename);
        Config parse();
};



#endif