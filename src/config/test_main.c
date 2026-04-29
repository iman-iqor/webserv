#include "Parser.hpp"
#include "Config.hpp"
#include <iostream>
#include <vector>


// config callParser(std::string filename)
// {
//     std::string content = filename;
//     std::vector<Token> tokens = tokenize(content);
//     Parser configParser(tokens);
//     Config conf = configParser.parse();
//     return conf;
// }
// Helper function to print the parsed data
void printParsedConfig(const Config& config) {
    std::cout << "===========================================" << std::endl;
    std::cout << "          PARSED CONFIGURATION            " << std::endl;
    std::cout << "===========================================" << std::endl;

    for (size_t i = 0; i < config.servers.size(); ++i) {
        const ServerBlock& s = config.servers[i];
        std::cout << "\n[SERVER BLOCK " << i << "]" << std::endl;
        
        // Listen Directives
        std::cout << "  Listen: ";
        for (size_t j = 0; j < s.listen_directives.size(); ++j) {
            std::cout << s.listen_directives[j].first << ":" << s.listen_directives[j].second << " ";
        }
        std::cout << std::endl;

        // Server Names
        std::cout << "  Server Names: ";
        for (size_t j = 0; j < s.server_names.size(); ++j) {
            std::cout << s.server_names[j] << " ";
        }
        std::cout << std::endl;

        std::cout << "  Root: " << s.root << std::endl;
        std::cout << "  Max Body Size: " << s.client_max_body_size << " bytes" << std::endl;

        // Error Pages
        std::cout << "  Error Pages: " << std::endl;
        std::map<int, std::string>::const_iterator it_err;
        for (it_err = s.error_pages.begin(); it_err != s.error_pages.end(); ++it_err) {
            std::cout << "    - " << it_err->first << " -> " << it_err->second << std::endl;
        }

        // Locations
        for (size_t j = 0; j < s.locations.size(); ++j) {
            const Location& l = s.locations[j];
            std::cout << "  \n  [LOCATION: " << l.path << "]" << std::endl;
            std::cout << "    Root: " << l.root << std::endl;
            std::cout << "    Index: " << l.index << std::endl;
            std::cout << "    Autoindex: " << (l.autoindex ? "on" : "off") << std::endl;
            std::cout << "    Upload Path: " << l.upload_path << std::endl;
            
            std::cout << "    Methods: ";
            for (size_t k = 0; k < l.methods.size(); ++k) std::cout << l.methods[k] << " ";
            std::cout << std::endl;

            if (l.return_code != 0)
                std::cout << "    Return: " << l.return_code << " -> " << l.return_url << std::endl;

            if (!l.cgi.empty()) {
                std::cout << "    CGI Handlers:" << std::endl;
                std::map<std::string, std::string>::const_iterator it_cgi;
                for (it_cgi = l.cgi.begin(); it_cgi != l.cgi.end(); ++it_cgi) {
                    std::cout << "      " << it_cgi->first << " -> " << it_cgi->second << std::endl;
                }
            }
        }
    }
    std::cout << "\n===========================================" << std::endl;
}

int main(int ac, char** av) {
    if (ac != 2) {
        std::cerr << "Usage: ./webserv [config_file]" << std::endl;
        return 1;
    }

    try {
        Parser configParser(av[1]);
        Config conf = configParser.parse();
        printParsedConfig(conf);
        std::cout << "Parsing successful!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error during parsing: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}


