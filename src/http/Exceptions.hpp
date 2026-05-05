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
    std::string _message;
public:
    BadRequestException(std::string message = "Bad Request") : _message(message) {}
    ~BadRequestException() throw() {}

    virtual const char* what() const throw() {
        return _message.c_str();
    }
};

class NotEmplementedException : public std::exception {
    std::string _message;
public:
    NotEmplementedException(std::string message = "Not Implemented") : _message(message) {}
    ~NotEmplementedException() throw() {}

    virtual const char* what() const throw() {
        return _message.c_str();
    }
};

class NotFoundException : public std::exception {
    std::string _message;
public:
    NotFoundException(std::string message = "Not Found") : _message(message) {}
    ~NotFoundException() throw() {}

    virtual const char* what() const throw() {
        return _message.c_str();
    }
};

class MethodNotAllowedException : public std::exception {
    std::string _message;
public:
    MethodNotAllowedException(std::string message = "Method Not Allowed") : _message(message) {}
    ~MethodNotAllowedException() throw() {}

    virtual const char* what() const throw() {
        return _message.c_str();
    }
};

class InternalServerErrorException : public std::exception {
    std::string _message;
public:
    InternalServerErrorException(std::string message = "Internal Server Error") : _message(message) {}
    ~InternalServerErrorException() throw() {}

    virtual const char* what() const throw() {
        return _message.c_str();
    }
};

class ForbiddenException : public std::exception {
    std::string _message;
public:
    ForbiddenException(std::string message = "Forbidden") : _message(message) {}
    ~ForbiddenException() throw() {}

    virtual const char* what() const throw() {
        return _message.c_str();
    }
};

class RedirectException : public std::exception {
    std::string _message;
public:
    RedirectException(std::string message = "Moved Permanently") : _message(message) {}
    ~RedirectException() throw() {}

    virtual const char* what() const throw() {
        return _message.c_str();
    }
};

class ServerException : public std::exception {
    std::string _message;
public:
    ServerException(std::string message = "Server Error") : _message(message) {}
    ~ServerException() throw() {}
    
    virtual const char* what() const throw() {
        return _message.c_str();
    }
};

class LengthRequiredException : public std::exception {
    std::string _message;
public:
    LengthRequiredException(std::string message = "Length Required") : _message(message) {}
    ~LengthRequiredException() throw() {}
    
    virtual const char* what() const throw() {
        return _message.c_str();
    }
};

#endif // EXCEPTIONS_HPP