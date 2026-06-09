#ifndef TEST_UTILS_HPP
#define TEST_UTILS_HPP

#include <iostream>
#include <string>
#include <vector>
#include "../../utils/colors.h"

struct TestResult {
    std::string name;
    bool passed;
    std::string message;
};

class MockCgiHandler {
public:
	static CgiState_t *MockStart(
		Client *client,
		const std::string &cgi_path,
		std::string &bin_path,
		const std::map< std::string, std::string > &env_map
	);
};

extern std::vector<TestResult> test_results;

void print_test_header(const std::string& category);
void print_test_start(const std::string& test_name);
void print_pass(const std::string& msg = "");
void print_fail(const std::string& msg = "");
void print_expected_exception(const std::string& exception_type);
void add_result(const std::string& test_name, bool passed, const std::string& message = "");
void run_test_cgi(void);
void print_summary();


#endif // TEST_UTILS_HPP
