#include "tests/test_utils.hpp"
#include "Request.hpp"
#include "Exceptions.hpp"
#include <unistd.h>
#include <fcntl.h>

// Forward declarations of test runners
extern void run_header_tests();
extern void run_cookie_tests();
extern void run_request_tests();

int main() {
    std::cout << BOLD_MAGENTA;
    std::cout << "\n╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║     WEBSERV HTTP PARSER - COMPREHENSIVE TEST SUITE   ║\n";
    std::cout << "║    Testing Header, Cookie, and Request Classes      ║\n";
    std::cout << "╚══════════════════════════════════════════════════════╝\n";
    std::cout << RESET;

    // Run all test suites
    run_header_tests();
    run_cookie_tests();
    run_request_tests();

    // Print summary
    print_summary();

    Request req;

    // int fd = open("test.txt", O_RDONLY);
    // char s[] = "POST /update-profile HTTP/1.1\r\nHost: localhost:8080\r\nUser-Agent: Mozilla/5.0\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: 23\r\nCookie: sessionid=abc123xyz; theme=dark; last_visit=2026-04-03\r\nConnection: keep-alive\r\n\r\nname=Radouane&age=22";
    // req.append_to_buffer(s);
    // try {
    //     req._parser();
    // } catch (const std::exception& e) {
    //     std::cerr << "Error parsing request: " << e.what() << std::endl;
    // }
    // // while (!req.is_finished()) {
    // //     try {
    // //     } catch (const std::exception& e) {
    // //         std::cerr << "Error reading request: " << e.what() << std::endl;
    // //         break;
    // //     }
    // // }
    // close(fd);

    return 0;
}
