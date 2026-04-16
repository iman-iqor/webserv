#ifndef HEADER_HPP
#define HEADER_HPP

#include <iostream>
#include <string>
#include <map>
#include <exception>

#include "Exceptions.hpp"
#include "../utils/colors.h"
#include "../utils/utils.hpp"

class Header {
private:
    std::map<std::string, std::string> _headers;
    std::map<std::string, std::string> _cookies;

    void _parser( const std::string &s );
    void _cookies_parser( const std::string &s );
    void _header_pair_parser( const std::string &s, char del);
    void _cookie_pair_parser( const std::string &s, char del);

public:
    Header( std::string &s );
    ~Header( void );

    void validate_headers( void );

    std::string& getHeader( const std::string &key );
    std::string& getCookie( const std::string &key );
    bool hasHeader( const std::string &key ) const;
    bool hasCookie( const std::string &key ) const;

	class HeaderNotFound: public std::exception {
		virtual const char *what( void ) const throw();
	};

	class CookieNotFound: public std::exception {
		virtual const char *what( void ) const throw();
	};
};

#endif // HEADER_HPP