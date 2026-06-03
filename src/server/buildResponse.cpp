#include "Server.hpp"

std::string Server::buildErrorResponse(int code, const std::string &message)
{
    std::string body;

    body += "<html><body><h1>";
    body += intToString(code);
    body += " ";
    body += message;
    body += "</h1></body></html>";

    std::string response;

    response += "HTTP/1.1 ";
    response += intToString(code);
    response += " ";
    response += message;
    response += "\r\n";

    response += "Content-Type: text/html\r\n";

    response += "Content-Length: ";
    response += intToString(body.length());
    response += "\r\n";

    response += "Connection: close\r\n";
    response += "\r\n";

    response += body;

    return response;
}