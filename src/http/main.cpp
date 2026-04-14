#include "Header.hpp"

#include "Header.hpp"
#include <vector>
#include <iomanip>

void run_test(const std::string& test_name, std::string raw_request) {
    std::cout << BOLD_YELLOW << "Testing: " << test_name << RESET << std::endl;
    std::cout << "Input: [" << YELLOW << raw_request << RESET << "]" << std::endl;
    
    try {
        Header h(raw_request);
        std::cout << GREEN << "[PASS] Header parsed successfully." << RESET << std::endl;
        
        // Example of checking a specific expected value
        // std::cout << "Host is: " << h.getHeader("host") << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << RED << "[EXPECTED ERROR/FAIL] " << e.what() << RESET << std::endl;
    }
    std::cout << std::string(40, '-') << std::endl;
}

int main() {
    std::cout << BOLD_MAGENTA << "--- WEBSERV HEADER & COOKIE UNIT TESTS ---" << std::endl << std::endl;

    // --- 1. VALID CASES ---
    run_test("Standard Valid Header", 
        "Host: localhost:8080\r\n"
        "User-Agent: Mozilla/5.0\r\n"
        "Accept: text/html, */*\r\n\r\n");

    run_test("Header Case Insensitivity", 
        "HOST: localhost\r\n"
        "content-type: text/plain\r\n\r\n");

    run_test("Multi-value (Folded) Headers", 
        "Cache-Control: no-cache\r\n"
        "Cache-Control: no-store\r\n\r\n");

    run_test("Empty Header Value", 
        "X-Empty-Header: \r\n"
        "Host: localhost\r\n\r\n");

    // --- 2. COOKIE CASES ---
    run_test("Valid Cookies", 
        "Host: localhost\r\n"
        "Cookie: sessionid=abc123xyz; Theme=dark; user=radouane\r\n\r\n");

    run_test("Duplicate Cookies (First Wins)", 
        "Cookie: id=first; id=second\r\n\r\n");

    run_test("Cookie with Base64 (Multiple =)", 
        "Cookie: data=SGVsbG8==; user=42\r\n\r\n");

    // --- 3. BROKEN SYNTAX (Should throw BadRequestException) ---
    run_test("Illegal Space Before Colon", 
        "Host : localhost\r\n\r\n");

    run_test("Missing Colon", 
        "Host localhost\r\n\r\n");

    run_test("Empty Header Key", 
        ": emptykey\r\n\r\n");

    run_test("Broken Cookie Assignment", 
        "Cookie: sessionid; theme=dark\r\n\r\n");

    run_test("Cookie Starting with Semicolon", 
        "Cookie: ; sessionid=123\r\n\r\n");

    run_test("Space in Cookie Key", 
        "Cookie: session id=123\r\n\r\n");

    // --- 4. 42 SPECIALS ---
    run_test("Missing Host Header", 
        "User-Agent: curl/7.68.0\r\n\r\n");

    std::cout << BOLD_MAGENTA << "--- TESTS COMPLETE ---" << RESET << std::endl;

    return 0;
}

// int main() {
//     std::string headers = "Host:  example.com  \r\nUser-Agent: Mozilla/5.0\r\nAccept: text/html";
//     Header header(headers);
//     try {
//         std::cout << "\nValidating headers..." << std::endl;
//         std::string &host = header.getHeader("Host");
//         std::cout << "Host: " << host << std::endl;

//         std::string &userAgent = header.getHeader("User-Agent");
//         std::cout << "User-Agent: " << userAgent << std::endl;

//         std::string &accept = header.getHeader("Accept");
//         std::cout << "Accept: " << accept << std::endl;

//         // This will throw an exception since "Content-Type" is not in the headers
//         std::string &contentType = header.getHeader("Content-Type");
//         std::cout << "Content-Type: " << contentType << std::endl;
//     } catch (const std::exception &e) {
//         std::cerr << e.what() << std::endl;
//     }

//     {
//         std::string badHeaders = "User Agent: Mozilla/5.0\r\nAccept: text/html"; // Missing Host header
        
//         try {
//             std::cout << "\nValidating bad headers..." << std::endl;
//             Header badHeader(badHeaders);
//         } catch (const std::exception &e) {
//             std::cerr << e.what() << std::endl; // Should print "400 Bad Request"
//         }
//     }

//     {
//         std::string badHeaders = "Accept: text/html";

//         try {
//             std::cout << "\nValidating bad headers..." << std::endl;
//             Header badHeader(badHeaders);
//             badHeader.validate_headers();
//         } catch (const std::exception &e) {
//             std::cerr << e.what() << std::endl; // Should print "400 Bad Request"
//         }
//     }
// }