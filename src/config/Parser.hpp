#ifndef PARSER_HPP
#define PARSER_HPP

#include "Config.hpp"


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
        void parseListenDirective(ServerBlock &server);
        long parseSize(std::string &str);
        void parseErrorPages(ServerBlock &server);
        void validMethod(std::string &str, Location &loc);
    public:
        Parser(std::string filename);
        Config parse();
};



#endif