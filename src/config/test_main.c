#include "Parser.hpp"
#include "Config.hpp"
#include <iostream>
#include <vector>



int main(int ac, char** av) {
    if (ac != 2) {
        std::cerr << "Usage: ./webserv [config_file]" << std::endl;
        return 1;
    }

    try {
        Parser configParser(av[1]);
        Config conf = configParser.parse();
        std::cout << "Parsing successful!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error during parsing: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}


