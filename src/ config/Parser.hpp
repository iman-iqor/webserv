#ifndef PARSER_HPP
#define PARSER_HPP

#include "config.hpp"


class Parser
{
    private:
        std::vector<Token> tokens;
        size_t i;
        Config _config;
        
        void parseServer();
        void parseLocation(ServerBlock &server);
        void parseDirective(ServerBlock &server);
        void expectSemicolon();
        void parseListenDirective(ServerBlock &server);
        long parseSize(std::string &str);
        void parseErrorPages(ServerBlock &server);
    public:
        Parser(std::vector<Token> &tokens);
        Config parse();
};



#endif