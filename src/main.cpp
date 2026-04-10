#include<iostream>
#include<exception>
#include"config/config.hpp"
#include"server/server.hpp"
#include"http/Request.hpp"
#include"http/Response.hpp"

//FAKE WHAT OUMAIM WILL GIVE ME
config createMockConfig()
{
    Config config;
    serverBlock server;
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

    }
}