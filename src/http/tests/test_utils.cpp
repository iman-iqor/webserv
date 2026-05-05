#include "test_utils.hpp"

std::vector<TestResult> test_results;

void print_test_header(const std::string& category) {
    std::cout << "\n" << BOLD_BLUE << "▶ " << category << RESET << std::endl;
    std::cout << std::string(70, '-') << std::endl;
}

void print_test_start(const std::string& test_name) {
    std::cout << BOLD_YELLOW << "  [TEST] " << test_name << RESET << std::endl;
}

void print_pass(const std::string& msg) {
    std::cout << GREEN << "    ✓ PASS" << RESET;
    if (!msg.empty()) std::cout << " - " << msg;
    std::cout << std::endl;
}

void print_fail(const std::string& msg) {
    std::cout << RED << "    ✗ FAIL" << RESET;
    if (!msg.empty()) std::cout << " - " << msg;
    std::cout << std::endl;
}

void print_expected_exception(const std::string& exception_type) {
    std::cout << CYAN << "    ✓ Expected exception: " << exception_type << RESET << std::endl;
}

void add_result(const std::string& test_name, bool passed, const std::string& message) {
    TestResult result;
    result.name = test_name;
    result.passed = passed;
    result.message = message;
    test_results.push_back(result);
}

void print_summary() {
    std::cout << "\n" << BOLD_MAGENTA << "════════════════════════════════════════════════════════\n";
    std::cout << "                       TEST SUMMARY\n";
    std::cout << "════════════════════════════════════════════════════════" << RESET << std::endl;
    
    int total = test_results.size();
    int passed = 0;
    
    for (size_t i = 0; i < test_results.size(); i++) {
        if (test_results[i].passed) {
            std::cout << GREEN << "✓" << RESET << " " << test_results[i].name << std::endl;
            passed++;
        } else {
            std::cout << RED << "✗" << RESET << " " << test_results[i].name;
            if (!test_results[i].message.empty()) 
                std::cout << " (" << test_results[i].message << ")";
            std::cout << std::endl;
        }
    }
    
    std::cout << "\n" << BOLD_CYAN << "Results: " << passed << "/" << total << " passed" << RESET << std::endl;
    if (passed < total) {
        std::cout << RED << "  " << (total - passed) << " test(s) failed" << RESET << std::endl;
    }
    std::cout << std::endl;
}
