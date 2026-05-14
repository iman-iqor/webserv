#include <iostream>
#include <exception>
#include <csignal>
#include <unistd.h>
#include "Config.hpp"
#include "Parser.hpp"
#include "Server.hpp"

volatile sig_atomic_t g_shutdown = 0;

void signalHandler(int signum)
{
    (void)signum;
    if (!g_shutdown)
        write(STDERR_FILENO, "\nReceived interrupt signal. Shutting down...\n", 46);
    g_shutdown = 1;
}

int main(int argc, char **argv)
{
    std::string configPath;

    if (argc == 2)
        configPath = argv[1];
    else
    {
        std::cerr << "Usage: ./webserv [config file] !" << std::endl;
        return 1;
    }

    try
    {
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);

        Parser parser(configPath);
        Config config = parser.parse();

        Server server(config);
        server.setupSockets();
        server.initEpoll();
        server.start();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}