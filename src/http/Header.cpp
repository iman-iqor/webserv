#include "Header.hpp"

#ifndef VERBOS
# define VERBOS true
#endif

/**
 * name = token
 * token = 1*<any CHAR except CTLs or separators>
 */
bool is_valid_key(const std::string& s)
{
    std::string allowed = "!#$%&'*+-.^_`|~";
    for (size_t i = 0; i < s.length(); ++i) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (!(std::isalnum(c) || allowed.find(c) != std::string::npos))
            return false;
    }
    return true;
}

bool is_valid_cookie_value(const std::string& s)
{
    for (size_t i = 0; i < s.length(); ++i) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 31 || c == 127 || c == ';' || c == '"' || c == ',' || c == '\\')
            return false;
    }
    return true;
}

Header::Header( std::string &headers )
{
    if (VERBOS) std::cout << BOLD_MAGENTA << "[HEADER]" << RESET << " Constructing header parser" << std::endl;
    _parser(headers);
    validate_headers();
    if (VERBOS) std::cout << BOLD_MAGENTA << "[HEADER]" << RESET << " Header validation completed" << std::endl;
}

Header::~Header( void )
{
    if (VERBOS) std::cout << BOLD_MAGENTA << "[HEADER]" << RESET << " Destroying header parser" << std::endl;
}

void Header::validate_headers( void )
{
    if (_headers.empty() || _headers.find("host") == _headers.end())
        throw BadRequestException();
    if (VERBOS) std::cout << MAGENTA << "[HEADER]" << RESET << " Required headers are present" << std::endl;
}

std::string &Header::getHeader( const std::string &key )
{
    std::string lower_key = key;
    to_lower(lower_key);
    if (_headers.find(lower_key) == _headers.end())
        throw BadRequestException(std::string("Header not found: ") + key);
    return _headers[lower_key];
}

bool Header::hasHeader( const std::string &key ) const
{
    std::string lower_key = key;
    to_lower(lower_key);
    return _headers.find(lower_key) != _headers.end();
}

bool Header::hasCookie( const std::string &key ) const
{
    return _cookies.find(key) != _cookies.end();
}

std::string &Header::getCookie( const std::string &key )
{
    if (_cookies.find(key) == _cookies.end())
        throw CookieNotFound();
    return _cookies[key];
}

void Header::_parser( const std::string &s )
{
    std::string str = s.substr(0, s.find("\r\n\r\n"));
    size_t pos_a, pos_b;
    std::string header;

    pos_a = 0;
    pos_b = str.find("\r\n", pos_a);
    while (pos_a < str.length()) {

        if (pos_b == std::string::npos)
            pos_b = str.length();

        header = str.substr(pos_a, pos_b - pos_a);
        _header_pair_parser(header, ':');


        pos_a = pos_b + 2;
        pos_b = str.find("\r\n", pos_a);
        if (VERBOS) std::cout << MAGENTA << "[HEADER] parsed line: " << RESET << header << std::endl;
    }

    if (VERBOS) std::cout << MAGENTA << "[HEADER]" << RESET << " End of headers" << std::endl;
}

void Header::_header_pair_parser( const std::string &s, char del)
{
    size_t pos = s.find(del);
    if (pos == std::string::npos) {
        throw BadRequestException();
    }

    std::string key = s.substr(0, pos);
    to_lower(key);
    if (key.empty() || !is_valid_key(key)
        || (key == "host" && _headers.find("host") != _headers.end())) {
        throw BadRequestException();
    }

    std::string value = trim(s.substr(pos + 1));
    if (key == "host" && (value.empty() || has_any(value, " \t\r\n"))) {
        throw BadRequestException();
    }
    else if (has_any(value, "\r\n")) {
        throw BadRequestException();
    }
    
    if (key == "cookie") {
        if (VERBOS) std::cout << MAGENTA << "[HEADER]" << RESET << " Parsing Cookie header" << std::endl;
        _cookies_parser(value);
    }
    else if (_headers.find(key) != _headers.end() && _headers[key] != "") {
        _headers[key] += ", " + value;
        if (VERBOS) std::cout << MAGENTA << "[HEADER]" << RESET << " Merged duplicated header: " << key << std::endl;
    }
    else {
        _headers[key] = value;
        if (VERBOS) std::cout << MAGENTA << "[HEADER]" << RESET << " Stored header: " << key << "=" << value << std::endl;
    }
}

void Header::_cookies_parser( const std::string &s )
{
    std::string str = s.substr(0, s.find("\r\n"));
    size_t pos_a, pos_b;
    std::string cookie;

    pos_a = 0;
    pos_b = str.find(';', pos_a);
    while (true) {

        if (pos_b == std::string::npos)
            pos_b = str.length();

        cookie = trim(str.substr(pos_a, pos_b - pos_a));
        if (cookie.empty()) {
            throw BadRequestException();
        }
        _cookie_pair_parser(cookie, '=');

        if (pos_b == str.length() - 1) {
            throw BadRequestException();
        }

        if (pos_b == str.length())
            break;

        pos_a = pos_b + 1;
        pos_b = str.find(';', pos_a);
    }

    if (VERBOS) std::cout << GREEN << "[HEADER]" << RESET << " End of cookies" << std::endl;
}

void Header::_cookie_pair_parser( const std::string &s, char del)
{
    size_t pos = s.find(del);
    if (pos == std::string::npos) {
        throw BadRequestException();
    }

    std::string key = trim(s.substr(0, pos));
    if (key.empty() || !is_valid_key(key)) {
        throw BadRequestException();
    }
    std::string value = trim(s.substr(pos + 1));
    if (value.at(0) == '"' && value.at(value.length() - 1) == '"') {
        value = value.substr(1, value.length() - 2);
    }
    if (!is_valid_cookie_value(value)) {
        throw BadRequestException();
    }
    
    if (_cookies.find(key) == _cookies.end()) {
        _cookies[key] = value;
        if (VERBOS) std::cout << GREEN << "[HEADER] parsed cookie: " << RESET << key << "=" << value << std::endl;
    }
}

const char *Header::HeaderNotFound::what( void ) const throw()
{
    return "404 Not Found: Header not found";
}

const char *Header::CookieNotFound::what( void ) const throw()
{
    return "404 Not Found: Cookie not found";
}
