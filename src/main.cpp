#include <iostream>
#include <vector>
#include <string>
#include "Config.hpp"   // Oumaima's part
#include "Server.hpp"   // Imane's part

int main(int argc, char **argv) {
    
    std::string configPath;

    // 1. Determine which config file to use
    if (argc == 2) {
        configPath = argv[1];
    } else if (argc == 1) {
        configPath = "default.conf";
    } else {
        std::cerr << "Usage: ./webserv [configuration_file]" << std::endl;
        return 1;
    }

    try {
        // 2. OUMAIMA'S PART: Parse the file
        Config config;
        config.parseFile(configPath); 

        // 3. IMANE'S PART: Initialize the Engine
        Server webserv;
        
        std::cout << "--- Initializing Sockets ---" << std::endl;
        webserv.init(config); 

        std::cout << "--- Server Running with epoll ---" << std::endl;
        webserv.run(); 

    } catch (const std::exception &e) {
        // This catches any parsing errors from Oumaima 
        // or socket errors from Imane
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}