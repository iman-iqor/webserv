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
#include <unistd.h>


volatile sig_atomic_t g_shutdown = 0; // This flag will be set to true when a shutdown signal is received, allowing the server's main loop to exit 
void signalHandler(int signum) {
    (void)signum;  // Avoid unused parameter warning
    if (!g_shutdown)
        write(STDERR_FILENO, "\nReceived interrupt signal. Shutting down...\n", 46);
    g_shutdown = 1;
}

int main(int argc,char** argv)
{

    std::string configPath;

    if(argc == 2)
    {
        configPath = argv[1];
    }
    else if(argc == 1)
    {
        configPath = "config.conf";
    }
    else
    {
        std::cerr<<"Usage: ./webserv [config file] !"<<std::endl;
        return 1;
    }
    try
    {
        std::signal(SIGINT, signalHandler);   
        std::signal(SIGTERM, signalHandler);  // this is when the process receives a termination signal, 
        Parser parser(configPath);
        Config config = parser.parse();

        Server server(config);
        server.setupSockets();
        server.initEpoll();
        server.start();

    }
    catch(const std::exception &e)
    {
        std::cerr<<"Error: "<<e.what()<<std::endl;
        return 1;
    }
    return 0;
}

