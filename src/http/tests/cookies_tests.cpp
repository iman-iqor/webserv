#include "test_utils.hpp"
#include "../Header.hpp"
#include <cassert>

// ============================================================================
// COOKIE TESTS - VALID CASES
// ============================================================================

void test_cookie_single_valid() {
    print_test_start("Single valid cookie");
    try {
        std::string raw = "Host: localhost\r\nCookie: sessionid=abc123\r\n\r\n";
        Header h(raw);
        
        std::string& cookie = h.getCookie("sessionid");
        assert(cookie == "abc123");
        
        print_pass("Single cookie parsed and retrieved correctly");
        add_result("cookie_single_valid", true);
    } catch (const std::exception& e) {
        print_fail(e.what());
        add_result("cookie_single_valid", false, e.what());
    }
}

void test_cookie_multiple_cookies() {
    print_test_start("Multiple cookies separated by semicolon");
    try {
        std::string raw = "Host: localhost\r\nCookie: sessionid=abc123; theme=dark; user=radouane\r\n\r\n";
        Header h(raw);
        
        std::string& sid = h.getCookie("sessionid");
        assert(sid == "abc123");
        
        std::string& theme = h.getCookie("theme");
        assert(theme == "dark");
        
        std::string& user = h.getCookie("user");
        assert(user == "radouane");
        
        print_pass("Multiple cookies parsed and retrieved correctly");
        add_result("cookie_multiple_cookies", true);
    } catch (const std::exception& e) {
        print_fail(e.what());
        add_result("cookie_multiple_cookies", false, e.what());
    }
}

void test_cookie_multiple_cookie_headers() {
    print_test_start("Multiple Cookie headers (valid, nginx-like)");
    try {
        std::string raw = "Host: localhost\r\nCookie: sessionid=abc123\r\nCookie: theme=dark\r\n\r\n";
        Header h(raw);

        std::string& sid = h.getCookie("sessionid");
        assert(sid == "abc123");

        std::string& theme = h.getCookie("theme");
        assert(theme == "dark");

        print_pass("Multiple Cookie headers parsed correctly");
        add_result("cookie_multiple_cookie_headers", true);
    } catch (const std::exception& e) {
        print_fail(e.what());
        add_result("cookie_multiple_cookie_headers", false, e.what());
    }
}

void test_cookie_whitespace_trimming() {
    print_test_start("Cookie value whitespace trimming");
    try {
        std::string raw = "Host: localhost\r\nCookie: sessionid =  abc123  ; theme = dark  \r\n\r\n";
        Header h(raw);
        
        std::string& sid = h.getCookie("sessionid");
        assert(sid == "abc123");
        
        std::string& theme = h.getCookie("theme");
        assert(theme == "dark");
        
        print_pass("Cookie values trimmed correctly");
        add_result("cookie_whitespace_trimming", true);
    } catch (const std::exception& e) {
        print_fail(e.what());
        add_result("cookie_whitespace_trimming", false, e.what());
    }
}

void test_cookie_base64_value() {
    print_test_start("Cookie with Base64 value (contains =)");
    try {
        std::string raw = "Host: localhost\r\nCookie: data=SGVsbG8gV29ybGQ=; token=abc==\r\n\r\n";
        Header h(raw);
        
        std::string& data = h.getCookie("data");
        assert(data == "SGVsbG8gV29ybGQ=");
        
        std::string& token = h.getCookie("token");
        assert(token == "abc==");
        
        print_pass("Base64-encoded cookies with = padding handled correctly");
        add_result("cookie_base64_value", true);
    } catch (const std::exception& e) {
        print_fail(e.what());
        add_result("cookie_base64_value", false, e.what());
    }
}

void test_cookie_duplicate_name_first_wins() {
    print_test_start("Duplicate cookie names (first value wins)");
    try {
        std::string raw = "Host: localhost\r\nCookie: id=first; id=second\r\n\r\n";
        Header h(raw);
        
        std::string& id = h.getCookie("id");
        assert(id == "first");
        
        print_pass("First cookie value retained when duplicate names");
        add_result("cookie_duplicate_name_first_wins", true);
    } catch (const std::exception& e) {
        print_fail(e.what());
        add_result("cookie_duplicate_name_first_wins", false, e.what());
    }
}

void test_cookie_special_characters() {
    print_test_start("Cookie with special characters (URI-encoded)");
    try {
        std::string raw = "Host: localhost\r\nCookie: user=john%20doe; path=%2Fadmin\r\n\r\n";
        Header h(raw);
        
        std::string& user = h.getCookie("user");
        assert(user == "john%20doe");
        
        print_pass("Special characters in cookie values handled");
        add_result("cookie_special_characters", true);
    } catch (const std::exception& e) {
        print_fail(e.what());
        add_result("cookie_special_characters", false, e.what());
    }
}

// ============================================================================
// COOKIE TESTS - INVALID CASES (Should throw BadRequestException)
// ============================================================================

void test_cookie_missing_equals() {
    print_test_start("Cookie without equals sign (BadRequestException)");
    try {
        std::string raw = "Host: localhost\r\nCookie: sessionid\r\n\r\n";
        Header h(raw);
        
        print_fail("Should have thrown BadRequestException for cookie without =");
        add_result("cookie_missing_equals", false, "No exception thrown");
    } catch (const BadRequestException& e) {
        print_expected_exception("BadRequestException");
        add_result("cookie_missing_equals", true);
    } catch (const std::exception& e) {
        print_fail(std::string("Wrong exception: ") + e.what());
        add_result("cookie_missing_equals", false, e.what());
    }
}

void test_cookie_empty_key() {
    print_test_start("Cookie with empty key (BadRequestException)");
    try {
        std::string raw = "Host: localhost\r\nCookie: =value\r\n\r\n";
        Header h(raw);
        
        print_fail("Should have thrown BadRequestException for empty cookie key");
        add_result("cookie_empty_key", false, "No exception thrown");
    } catch (const BadRequestException& e) {
        print_expected_exception("BadRequestException");
        add_result("cookie_empty_key", true);
    } catch (const std::exception& e) {
        print_fail(std::string("Wrong exception: ") + e.what());
        add_result("cookie_empty_key", false, e.what());
    }
}

void test_cookie_leading_semicolon() {
    print_test_start("Cookie with leading semicolon (BadRequestException)");
    try {
        std::string raw = "Host: localhost\r\nCookie: ; sessionid=123\r\n\r\n";
        Header h(raw);
        
        print_fail("Should have thrown BadRequestException for leading semicolon");
        add_result("cookie_leading_semicolon", false, "No exception thrown");
    } catch (const BadRequestException& e) {
        print_expected_exception("BadRequestException");
        add_result("cookie_leading_semicolon", true);
    } catch (const std::exception& e) {
        print_fail(std::string("Wrong exception: ") + e.what());
        add_result("cookie_leading_semicolon", false, e.what());
    }
}

void test_cookie_space_in_key() {
    print_test_start("Cookie key with space (BadRequestException)");
    try {
        std::string raw = "Host: localhost\r\nCookie: session id=123\r\n\r\n";
        Header h(raw);
        
        print_fail("Should have thrown BadRequestException for space in cookie key");
        add_result("cookie_space_in_key", false, "No exception thrown");
    } catch (const BadRequestException& e) {
        print_expected_exception("BadRequestException");
        add_result("cookie_space_in_key", true);
    } catch (const std::exception& e) {
        print_fail(std::string("Wrong exception: ") + e.what());
        add_result("cookie_space_in_key", false, e.what());
    }
}

void test_cookie_tab_in_key() {
    print_test_start("Cookie key with tab (BadRequestException)");
    try {
        std::string raw = "Host: localhost\r\nCookie: session\tid=123\r\n\r\n";
        Header h(raw);
        
        print_fail("Should have thrown BadRequestException for tab in cookie key");
        add_result("cookie_tab_in_key", false, "No exception thrown");
    } catch (const BadRequestException& e) {
        print_expected_exception("BadRequestException");
        add_result("cookie_tab_in_key", true);
    } catch (const std::exception& e) {
        print_fail(std::string("Wrong exception: ") + e.what());
        add_result("cookie_tab_in_key", false, e.what());
    }
}

void test_cookie_trailing_semicolon() {
    print_test_start("Cookie with trailing semicolon (BadRequestException)");
    try {
        std::string raw = "Host: localhost\r\nCookie: sessionid=abc; \r\n\r\n";
        Header h(raw);

        print_fail("Should have thrown BadRequestException for trailing semicolon");
        add_result("cookie_trailing_semicolon", false, "No exception thrown");
    } catch (const BadRequestException& e) {
        print_expected_exception("BadRequestException");
        add_result("cookie_trailing_semicolon", true);
    } catch (const std::exception& e) {
        print_fail(std::string("Wrong exception: ") + e.what());
        add_result("cookie_trailing_semicolon", false, e.what());
    }
}

void test_cookie_illegal_chars() {
    print_test_start("Cookie with illegal characters (BadRequestException)");
    try {
        std::string raw = "Host: localhost\r\nCookie: id=abc,def\r\n\r\n";
        Header h(raw);

        print_fail("Should have thrown BadRequestException for illegal cookie chars");
        add_result("cookie_illegal_chars", false, "No exception thrown");
    } catch (const BadRequestException& e) {
        print_expected_exception("BadRequestException");
        add_result("cookie_illegal_chars", true);
    } catch (const std::exception& e) {
        print_fail(std::string("Wrong exception: ") + e.what());
        add_result("cookie_illegal_chars", false, e.what());
    }
}

// ============================================================================
// COOKIE TESTS - GETTERS (CookieNotFound exception)
// ============================================================================

void test_cookie_get_nonexistent() {
    print_test_start("Get non-existent cookie (CookieNotFound exception)");
    try {
        std::string raw = "Host: localhost\r\nCookie: sessionid=abc123\r\n\r\n";
        Header h(raw);
        
        std::string& cookie = h.getCookie("nonexistent");
        (void)cookie;  // silence unused variable warning
        
        print_fail("Should have thrown CookieNotFound");
        add_result("cookie_get_nonexistent", false, "No exception thrown");
    } catch (const Header::CookieNotFound& e) {
        print_expected_exception("CookieNotFound");
        add_result("cookie_get_nonexistent", true);
    } catch (const std::exception& e) {
        print_fail(std::string("Wrong exception: ") + e.what());
        add_result("cookie_get_nonexistent", false, e.what());
    }
}

void test_cookie_no_cookies_at_all() {
    print_test_start("Get cookie when no cookies present (CookieNotFound)");
    try {
        std::string raw = "Host: localhost\r\nUser-Agent: curl\r\n\r\n";
        Header h(raw);
        
        std::string& cookie = h.getCookie("any");
        (void)cookie;  // silence unused variable warning
        
        print_fail("Should have thrown CookieNotFound");
        add_result("cookie_no_cookies_at_all", false, "No exception thrown");
    } catch (const Header::CookieNotFound& e) {
        print_expected_exception("CookieNotFound");
        add_result("cookie_no_cookies_at_all", true);
    } catch (const std::exception& e) {
        print_fail(std::string("Wrong exception: ") + e.what());
        add_result("cookie_no_cookies_at_all", false, e.what());
    }
}

// ============================================================================
// COOKIE TEST RUNNER
// ============================================================================

void run_cookie_tests() {
    print_test_header("COOKIE TESTS - VALID CASES");
    test_cookie_single_valid();
    test_cookie_multiple_cookies();
    test_cookie_multiple_cookie_headers();
    test_cookie_whitespace_trimming();
    test_cookie_base64_value();
    test_cookie_duplicate_name_first_wins();
    test_cookie_special_characters();

    print_test_header("COOKIE TESTS - INVALID CASES (BadRequestException)");
    test_cookie_missing_equals();
    test_cookie_empty_key();
    test_cookie_leading_semicolon();
    test_cookie_trailing_semicolon();
    test_cookie_space_in_key();
    test_cookie_tab_in_key();
    test_cookie_illegal_chars();

    print_test_header("COOKIE TESTS - GETTER EXCEPTIONS");
    test_cookie_get_nonexistent();
    test_cookie_no_cookies_at_all();
}
