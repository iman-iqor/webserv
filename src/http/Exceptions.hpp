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
        return "Bad Request";
    }
};

class NotEmplementedException : public std::exception {
public:
    virtual const char* what() const throw() {
        return "Not Implemented";
    }
};

class NotFoundException : public std::exception {
public:
    virtual const char* what() const throw() {
        return "Not Found";
    }
};

class MethodNotAllowedException : public std::exception {
public:
    virtual const char* what() const throw() {
        return "Method Not Allowed";
    }
};

class InternalServerErrorException : public std::exception {
public:
    virtual const char* what() const throw() {
        return "Internal Server Error";
    }
};

class ForbiddenException : public std::exception {
public:
    virtual const char* what() const throw() {
        return "Forbidden";
    }
};

class RedirectException : public std::exception {
public:
    virtual const char* what() const throw() {
        return "Moved Permanently";
    }
};

#endif // EXCEPTIONS_HPP