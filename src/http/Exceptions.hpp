#ifndef EXCEPTIONS_HPP
#define EXCEPTIONS_HPP

#include <exception>

/**
 * ** 400 Bad Request: **
 * * Request line: *
 * Invalid method
 * Invalid URI
 * Invalid HTTP version
 * Missing spaces in request first line
 * 
 * * Headers: *
 * Missing Host header
 * Invalid header format
 * Header name with spaces
 * Multiple conflicting headers (e.g., multiple Content-Length headers with different values)
 * Transfer-Encoding not supported
 * 
 * ** body: **
 * Request body larger or smaller  than Content-Length
 * Request body in GET request
 * 
 */
class BadRequestException : public std::exception {
public:
    virtual const char* what() const throw() {
        return "400 Bad Request";
    }
};

class NotEmplementedException : public std::exception {
public:
    virtual const char* what() const throw() {
        return "501 Not Implemented";
    }
};

#endif // EXCEPTIONS_HPP