#ifndef EXCEPTIONS_HPP
#define EXCEPTIONS_HPP

#include <exception>

class HttpException : public std::exception {
public:
    int         statusCode;
    std::string message;
    std::string statusMessage;

    HttpException(int code, const std::string& msg, const std::string& statusMsg)
        : statusCode(code), message(msg), statusMessage(statusMsg) {}
    ~HttpException() throw() {}

    const char* what() const throw() {
        return message.c_str();
    }
};

/* 4xx */

class BadRequestException : public HttpException {
public:    BadRequestException(const std::string& msg = "Bad Request")
        : HttpException(400, msg, "Bad Request") {}
};

class ForbiddenException : public HttpException {
public:    ForbiddenException(const std::string& msg = "Forbidden")
        : HttpException(403, msg, "Forbidden") {}
};

class NotFoundException : public HttpException {
public:    NotFoundException(const std::string& msg = "Not Found")
        : HttpException(404, msg, "Not Found") {}
};

class MethodNotAllowedException : public HttpException {
public:    MethodNotAllowedException(const std::string& msg = "Method Not Allowed")
        : HttpException(405, msg, "Method Not Allowed") {}
};

class LengthRequiredException : public HttpException {
public:    LengthRequiredException(const std::string& msg = "Length Required")
        : HttpException(411, msg, "Length Required") {}
};

class PayloadTooLargeException : public HttpException {
public:    PayloadTooLargeException(const std::string& msg = "Payload Too Large")
        : HttpException(413, msg, "Payload Too Large") {}
};

class UnsupportedMediaTypeException : public HttpException {
public:    UnsupportedMediaTypeException(const std::string& msg = "Unsupported Media Type")
        : HttpException(415, msg, "Unsupported Media Type") {}
};

/* 5xx */

class InternalServerErrorException : public HttpException {
public:    InternalServerErrorException(const std::string& msg = "Internal Server Error")
        : HttpException(500, msg, "Internal Server Error") {}
};

class NotImplementedException : public HttpException {
public:    NotImplementedException(const std::string& msg = "Not Implemented")
        : HttpException(501, msg, "Not Implemented") {}
};

class BadGatewayException : public HttpException {
public:    BadGatewayException(const std::string& msg = "Bad Gateway")
        : HttpException(502, msg, "Bad Gateway") {}
};

#endif // EXCEPTIONS_HPP