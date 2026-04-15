#include "tests/test_utils.hpp"
#include "Header.hpp"
#include "Exceptions.hpp"

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

    // std::string s = "Host: localhost\r\nCookie: sessionid=\"abc123\"; theme=dark\r\n\r\n";
    // Header h(s);



    // Run all test suites
    run_header_tests();
    run_cookie_tests();
    run_request_tests();

    // Print summary
    print_summary();

    return 0;
}
