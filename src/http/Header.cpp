#include "Header.hpp"

#define DEBUG_MODE 1

Header::Header( std::string &headers )
{
    if (DEBUG_MODE) std::cout << YELLOW << "[ DEBUG ]:" << RESET << " Header constructor called" << std::endl;
    _parser(headers);
}

Header::~Header( void )
{
    if (DEBUG_MODE) std::cout << YELLOW << "[ DEBUG ]:" << RESET << " Header destructor called" << std::endl;
}

void Header::validate_headers( void )
{
    if (_headers.empty() || _headers.find("Host") == _headers.end())
        throw BadRequestException();
}

std::string &Header::getHeader( const std::string &key )
{
    if (_headers.find(key) == _headers.end())
        throw HeaderNotFound();
    return _headers[key];
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
    ssize_t pos_a, pos_b;
    std::string key, value, header;

    pos_a = 0;
    pos_b = str.find("\r\n", pos_a);
    while (pos_a < str.length()) {

        if (pos_b == std::string::npos)
            pos_b = str.length();

        header = str.substr(pos_a, pos_b - pos_a);
        _header_pair_parser(header, ':');


        pos_a = pos_b + 2;
        pos_b = str.find("\r\n", pos_a);
        if (DEBUG_MODE) std::cout << MAGENTA << "H: " << RESET << header << std::endl;
    }

    if (DEBUG_MODE) std::cout << "End of headers" << std::endl;
}

void Header::_header_pair_parser( const std::string &s, char del)
{
    ssize_t pos = s.find(del);
    if (pos == std::string::npos) {
        throw BadRequestException();
    }

    std::string key = s.substr(0, pos);
    to_lower(key);
    if (key.empty() || has_any(key, " \t\r\n")
        || (key == "host" && _headers.find("host") != _headers.end())) {
        throw BadRequestException();
    }

    std::string value = trim(s.substr(pos + 1));
    if (key == "host" && (value.empty() || has_any(value, " \t\r\n"))) {
        throw BadRequestException();
    }
    
    if (key == "cookie") {
        _cookies_parser(value);
    }
    else if (_headers.find(key) != _headers.end() && _headers[key] != "") {
        _headers[key] += ", " + value;
    }
    else {
        _headers[key] = value;
    }
}

void Header::_cookies_parser( const std::string &s )
{
    ssize_t pos_a, pos_b;
    std::string key, value, cookie;

    pos_a = 0;
    pos_b = s.find("\r\n", pos_a);
    while (pos_a < s.length()) {

        if (pos_b == std::string::npos)
            pos_b = s.length();

        cookie = s.substr(pos_a, pos_b - pos_a);
        _cookie_pair_parser(cookie, '=');

        pos_a = pos_b + 1;
        pos_b = s.find(';', pos_a);
        if (DEBUG_MODE) std::cout << MAGENTA << "C: " << RESET << cookie << std::endl;
    }

    if (DEBUG_MODE) std::cout << "End of cookies" << std::endl;
}

/**
 * TODO: watch for Illegal Characters
 */
void Header::_cookie_pair_parser( const std::string &s, char del)
{
    ssize_t pos = s.find(del);
    if (pos == std::string::npos) {
        throw BadRequestException();
    }

    std::string key = s.substr(0, pos);
    if (key.empty() || has_any(key, " \t\r\n")) {
        throw BadRequestException();
    }
    std::string value = trim(s.substr(pos + 1));
    
    if (_cookies.find(key) == _cookies.end()) {
        _cookies[key] = value;
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
