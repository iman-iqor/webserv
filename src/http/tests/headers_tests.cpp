#include "test_utils.hpp"
#include "../Header.hpp"
#include <cassert>

// ============================================================================
// HEADER TESTS - VALID CASES
// ============================================================================

void test_header_standard_valid() {
    print_test_start("Standard valid header with multiple fields");
    try {
        std::string raw = "Host: localhost:8080\r\nUser-Agent: Mozilla/5.0\r\nAccept: text/html\r\n\r\n";
        Header h(raw);
        
        std::string& host = h.getHeader("host");
        assert(host == "localhost:8080");
        
        std::string& ua = h.getHeader("user-agent");
        assert(ua == "Mozilla/5.0");
        
        print_pass("All headers parsed correctly, host and user-agent verified");
        add_result("header_standard_valid", true);
    } catch (const std::exception& e) {
        print_fail(e.what());
        add_result("header_standard_valid", false, e.what());
    }
}

void test_header_case_insensitivity() {
    print_test_start("Header names case insensitivity (nginx behavior)");
    try {
        std::string raw = "HOST: example.com\r\nContent-Type: text/plain\r\nConTenT-LeNgTh: 42\r\n\r\n";
        Header h(raw);
        
        std::string& host = h.getHeader("host");
        assert(host == "example.com");
        
        std::string& ct = h.getHeader("content-type");
        assert(ct == "text/plain");
        
        std::string& cl = h.getHeader("content-length");
        assert(cl == "42");
        
        print_pass("Header names are case-insensitive, all retrieved correctly");
        add_result("header_case_insensitivity", true);
    } catch (const std::exception& e) {
        print_fail(e.what());
        add_result("header_case_insensitivity", false, e.what());
    }
}

void test_header_whitespace_trimming() {
    print_test_start("Whitespace trimming in header values (nginx behavior)");
    try {
        std::string raw = "Host:   localhost   \r\nUser-Agent:    curl/7.68.0   \r\n\r\n";
        Header h(raw);
        
        std::string& host = h.getHeader("host");
        assert(host == "localhost");
        
        std::string& ua = h.getHeader("user-agent");
        assert(ua == "curl/7.68.0");
        
        print_pass("Whitespace properly trimmed from values");
        add_result("header_whitespace_trimming", true);
    } catch (const std::exception& e) {
        print_fail(e.what());
        add_result("header_whitespace_trimming", false, e.what());
    }
}

void test_header_empty_value() {
    print_test_start("Empty header value (nginx allows)");
    try {
        std::string raw = "Host: example.com\r\nX-Empty-Header: \r\n\r\n";
        Header h(raw);
        
        std::string& empty = h.getHeader("x-empty-header");
        assert(empty == "");
        
        print_pass("Empty header value accepted");
        add_result("header_empty_value", true);
    } catch (const std::exception& e) {
        print_fail(e.what());
        add_result("header_empty_value", false, e.what());
    }
}

void test_header_multiple_identical_headers() {
    print_test_start("Multiple identical headers (concatenated with comma)");
    try {
        std::string raw = "Host: example.com\r\nCache-Control: no-cache\r\nCache-Control: no-store\r\n\r\n";
        Header h(raw);
        
        std::string& cc = h.getHeader("cache-control");
        assert(cc.find("no-cache") != std::string::npos);
        assert(cc.find("no-store") != std::string::npos);
        
        print_pass("Multiple headers concatenated with comma");
        add_result("header_multiple_identical_headers", true);
    } catch (const std::exception& e) {
        print_fail(e.what());
        add_result("header_multiple_identical_headers", false, e.what());
    }
}

void test_header_special_characters_in_value() {
    print_test_start("Special characters in header values");
    try {
        std::string raw = "Host: example.com\r\nAuthorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9\r\n\r\n";
        Header h(raw);
        
        std::string& auth = h.getHeader("authorization");
        assert(auth.find("Bearer") != std::string::npos);
        
        print_pass("Special characters and Base64 in values handled correctly");
        add_result("header_special_characters_in_value", true);
    } catch (const std::exception& e) {
        print_fail(e.what());
        add_result("header_special_characters_in_value", false, e.what());
    }
}

// ============================================================================
// HEADER TESTS - INVALID CASES (Should throw BadRequestException)
// ============================================================================

void test_header_missing_host() {
    print_test_start("Missing Host header (BadRequestException)");
    try {
        std::string raw = "User-Agent: curl/7.68.0\r\nAccept: */*\r\n\r\n";
        Header h(raw);
        
        print_fail("Should have thrown BadRequestException for missing Host");
        add_result("header_missing_host", false, "No exception thrown");
    } catch (const BadRequestException& e) {
        print_expected_exception("BadRequestException");
        add_result("header_missing_host", true);
    } catch (const std::exception& e) {
        print_fail(std::string("Wrong exception: ") + e.what());
        add_result("header_missing_host", false, e.what());
    }
}

void test_header_missing_colon() {
    print_test_start("Missing colon in header (BadRequestException)");
    try {
        std::string raw = "Host localhost\r\n\r\n";
        Header h(raw);
        
        print_fail("Should have thrown BadRequestException for missing colon");
        add_result("header_missing_colon", false, "No exception thrown");
    } catch (const BadRequestException& e) {
        print_expected_exception("BadRequestException");
        add_result("header_missing_colon", true);
    } catch (const std::exception& e) {
        print_fail(std::string("Wrong exception: ") + e.what());
        add_result("header_missing_colon", false, e.what());
    }
}

void test_header_space_before_colon() {
    print_test_start("Space before colon in header (BadRequestException)");
    try {
        std::string raw = "Host : localhost\r\n\r\n";
        Header h(raw);
        
        print_fail("Should have thrown BadRequestException for space before colon");
        add_result("header_space_before_colon", false, "No exception thrown");
    } catch (const BadRequestException& e) {
        print_expected_exception("BadRequestException");
        add_result("header_space_before_colon", true);
    } catch (const std::exception& e) {
        print_fail(std::string("Wrong exception: ") + e.what());
        add_result("header_space_before_colon", false, e.what());
    }
}

void test_header_empty_key() {
    print_test_start("Empty header key (BadRequestException)");
    try {
        std::string raw = ": emptykey\r\nHost: localhost\r\n\r\n";
        Header h(raw);
        
        print_fail("Should have thrown BadRequestException for empty key");
        add_result("header_empty_key", false, "No exception thrown");
    } catch (const BadRequestException& e) {
        print_expected_exception("BadRequestException");
        add_result("header_empty_key", true);
    } catch (const std::exception& e) {
        print_fail(std::string("Wrong exception: ") + e.what());
        add_result("header_empty_key", false, e.what());
    }
}

void test_header_duplicate_host() {
    print_test_start("Duplicate Host header (BadRequestException)");
    try {
        std::string raw = "Host: example.com\r\nHost: other.com\r\n\r\n";
        Header h(raw);
        
        print_fail("Should have thrown BadRequestException for duplicate Host");
        add_result("header_duplicate_host", false, "No exception thrown");
    } catch (const BadRequestException& e) {
        print_expected_exception("BadRequestException");
        add_result("header_duplicate_host", true);
    } catch (const std::exception& e) {
        print_fail(std::string("Wrong exception: ") + e.what());
        add_result("header_duplicate_host", false, e.what());
    }
}

void test_header_tab_in_key() {
    print_test_start("Tab in header key (BadRequestException)");
    try {
        std::string raw = "Host\there: localhost\r\n\r\n";
        Header h(raw);
        
        print_fail("Should have thrown BadRequestException for tab in key");
        add_result("header_tab_in_key", false, "No exception thrown");
    } catch (const BadRequestException& e) {
        print_expected_exception("BadRequestException");
        add_result("header_tab_in_key", true);
    } catch (const std::exception& e) {
        print_fail(std::string("Wrong exception: ") + e.what());
        add_result("header_tab_in_key", false, e.what());
    }
}

void test_header_newline_in_key() {
    print_test_start("Newline character in header key (BadRequestException)");
    try {
        std::string raw = "Host\nname: localhost\r\n\r\n";
        Header h(raw);
        
        print_fail("Should have thrown BadRequestException for newline in key");
        add_result("header_newline_in_key", false, "No exception thrown");
    } catch (const BadRequestException& e) {
        print_expected_exception("BadRequestException");
        add_result("header_newline_in_key", true);
    } catch (const std::exception& e) {
        print_fail(std::string("Wrong exception: ") + e.what());
        add_result("header_newline_in_key", false, e.what());
    }
}

// ============================================================================
// HEADER TESTS - GETTERS (HeaderNotFound exception)
// ============================================================================

void test_header_get_nonexistent() {
    print_test_start("Get non-existent header (HeaderNotFound exception)");
    try {
        std::string raw = "Host: example.com\r\n\r\n";
        Header h(raw);
        
        std::string& header = h.getHeader("Content-Type");
        (void)header;  // silence unused variable warning
        
        print_fail("Should have thrown HeaderNotFound");
        add_result("header_get_nonexistent", false, "No exception thrown");
    } catch (const Header::HeaderNotFound& e) {
        print_expected_exception("HeaderNotFound");
        add_result("header_get_nonexistent", true);
    } catch (const std::exception& e) {
        print_fail(std::string("Wrong exception: ") + e.what());
        add_result("header_get_nonexistent", false, e.what());
    }
}

// ============================================================================
// HEADER TEST RUNNER
// ============================================================================

void run_header_tests() {
    print_test_header("HEADER TESTS - VALID CASES (nginx-compliant)");
    test_header_standard_valid();
    test_header_case_insensitivity();
    test_header_whitespace_trimming();
    test_header_empty_value();
    test_header_multiple_identical_headers();
    test_header_special_characters_in_value();

    print_test_header("HEADER TESTS - INVALID CASES (BadRequestException)");
    test_header_missing_host();
    test_header_missing_colon();
    test_header_space_before_colon();
    test_header_empty_key();
    test_header_duplicate_host();
    test_header_tab_in_key();
    test_header_newline_in_key();

    print_test_header("HEADER TESTS - GETTER EXCEPTIONS");
    test_header_get_nonexistent();
}
