#include<iostream>
#include<exception>
#include"config/Config.hpp"
#include"server/Server.hpp"
#include"http/Request.hpp"
#include"http/Response.hpp"

//FAKE WHAT OUMAIM WILL GIVE ME
Config createMockConfig()
{
    Config config;
    ServerBlock server;
    server.listen_directives.push_back(std::make_pair("127.0.0.1", 8080));
    server.server_names.push_back("example.com");//ths is just for testing the print function
    server.root="./www";

    config.servers.push_back(server);
    return config;
}


int main(int argc,char** argv)
{


    //DETERMINE CONFIG FILE
    std::string configPath;

    if(argc == 2)
    {
        configPath = argv[1];
        std::cout<<"using config: "<<configPath<<std::endl;
    }
    else if(argc == 1)
    {
        configPath = "webserv.conf";
        std::cout<<"using default config: "<<configPath<<std::endl;
    }
    else
    {
        std::cerr<<"Usage: ./weberv [config file] !"<<std::endl;
        return 1;
    }
    try
    {
        //OUMAIMA PARSING CONFIG FILE
        Config config;
        //config.parseFile(configPath) //not available for now
        config = createMockConfig();
        
        //multiplexing and server setup
        Server server(config);
        server.start();
    }
    catch(const std::exception &e)
    {
        std::cerr<<"Error: "<<e.what()<<std::endl;
        return 1;
    }
    return 0;
}