#include "Header.hpp"

int main() {
    std::string headers = "Host:  example.com  \r\nUser-Agent: Mozilla/5.0\r\nAccept: text/html";
    Header header(headers);
    try {
        std::cout << "\nValidating headers..." << std::endl;
        std::string &host = header.getHeader("Host");
        std::cout << "Host: " << host << std::endl;

        std::string &userAgent = header.getHeader("User-Agent");
        std::cout << "User-Agent: " << userAgent << std::endl;

        std::string &accept = header.getHeader("Accept");
        std::cout << "Accept: " << accept << std::endl;

        // This will throw an exception since "Content-Type" is not in the headers
        std::string &contentType = header.getHeader("Content-Type");
        std::cout << "Content-Type: " << contentType << std::endl;
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }

    {
        std::string badHeaders = "User Agent: Mozilla/5.0\r\nAccept: text/html"; // Missing Host header
        
        try {
            std::cout << "\nValidating bad headers..." << std::endl;
            Header badHeader(badHeaders);
        } catch (const std::exception &e) {
            std::cerr << e.what() << std::endl; // Should print "400 Bad Request"
        }
    }

    {
        std::string badHeaders = "Accept: text/html";

        try {
            std::cout << "\nValidating bad headers..." << std::endl;
            Header badHeader(badHeaders);
            badHeader.validate_headers();
        } catch (const std::exception &e) {
            std::cerr << e.what() << std::endl; // Should print "400 Bad Request"
        }
    }
}