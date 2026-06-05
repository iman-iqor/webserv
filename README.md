🌐 Webserv

A custom HTTP/1.1 web server written in C++98 as part of the 42 curriculum.
It supports routing, static file serving, CGI execution, and multiple servers using non-blocking I/O with select()/poll()/epoll().

📋 Table of Contents
About
Features
Prerequisites
Installation
Usage
Configuration
Supported Features
HTTP Methods
CGI Support
Testing
Project Architecture
Authors
📌 About

Webserv is a lightweight HTTP server that mimics basic behavior of production servers like Nginx.

It handles:

HTTP request parsing
Routing based on configuration
Static file serving
Directory listing (autoindex)
CGI execution
Multiple clients concurrently using I/O multiplexing
✨ Features
HTTP/1.1 compliant request handling
GET / POST / DELETE support
Static file serving
Directory listing (autoindex)
Custom error pages
Multiple server blocks (virtual hosts)
Host & port-based routing
CGI execution (Python / PHP support)
Client body size limitation
Non-blocking I/O with select() / poll()
Keep-alive support (if implemented)
🔧 Prerequisites
OS: Linux / macOS
Compiler: g++ or clang++ (C++98 standard)
Make
Optional:
Python (for CGI)
PHP-CGI (for CGI testing)

📦 Installation
git clone https://github.com/iman-iqor/webserv
cd webserv
make

🚀 Usage
./webserv config.conf
Or use default config:
./webserv

🌐 Supported Features
📄 Static Files
HTML, CSS, JS, images
Proper MIME-type handling
📁 Autoindex

If no index file exists:

Generates directory listing HTML
🚨 Error Handling
400 Bad Request
403 Forbidden
404 Not Found
405 Method Not Allowed
413 Payload Too Large
500 Internal Server Error


🔄 HTTP Methods GET POST DELETE


🧬 CGI Support

Supports execution of external scripts:
Python (.py)
PHP (.php)
curl -X POST http://localhost:8080/cgi-bin/test.py


🧠 Project Architecture
File not showing in browser
Check MIME type
Check Content-Length
Ensure file exists
Autoindex not working
Directory must exist
No index file present
autoindex must be ON
CGI not working
Script must be executable or mapped correctly
Correct interpreter path required
👥 Authors

Developed by:

👨‍💻 imiqor
👩‍💻 oumaima
👩‍💻 radouane

At 1337/42 / UM6P

📄 License

This project is part of the 42 School curriculum.
