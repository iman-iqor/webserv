#include<iostream>
#include<exception>
#include"src/config/Config.hpp"
#include"src/config/Parser.hpp"
#include"src/server/Server.hpp"
#include"src/server/Client.hpp"
#include"src/http/Request.hpp"
#include"src/http/Response.hpp"
#include <signal.h>// This is for signal handling to gracefully shut down the server on SIGINT (Ctrl+C)
#include <csignal>// This is for signal handling to gracefully shut down the server on SIGINT (Ctrl+C)

// ✅ Global flag for signal handling


// ✅ Signal handler
void signalHandler(int signum) {
    (void)signum;  // Avoid unused parameter warning
    std::cout << "\nReceived interrupt signal. Shutting down..." << std::endl;
    g_shutdown = true;
}

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
        signal(SIGINT, signalHandler);   // Ctrl+C
        signal(SIGTERM, signalHandler);  // Termination
        //oumaima
        Parser parser(configPath);
        Config config = parser.parse();
        
        
        
        //imane
        Server server(config);
        server.setupSockets();
        server.initEpoll();
        server.start();

        //redouane
    }
    catch(const std::exception &e)
    {
        std::cerr<<"Error: "<<e.what()<<std::endl;
        return 1;
    }
    return 0;
}

