#include<iostream>
#include<exception>
#include"src/config/Config.hpp"
#include"src/config/Parser.hpp"
#include"src/server/Server.hpp"
#include"src/http/Request.hpp"
#include"src/http/Response.hpp"
int main(int argc,char** argv)
{

    std::string configPath;

    if(argc == 2)
    {
        configPath = argv[1];
    }
    else
    {
        std::cerr<<"Usage: ./webserv [config file] !"<<std::endl;
        return 1;
    }
    try
    {
        //oumaima
        Parser parser(configPath);
        Config config = parser.parse();
        
        
        
        //imane
        Server server(config);
        server.setupSockets();

        //redouane
    }
    catch(const std::exception &e)
    {
        std::cerr<<"Error: "<<e.what()<<std::endl;
        return 1;
    }
    return 0;
}

