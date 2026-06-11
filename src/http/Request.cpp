#include "Request.hpp"
#include <unistd.h>
#include <cstdlib>
#include <sys/socket.h>
#include <algorithm>
#include "../../webserv.h"

#define BUFFER_SIZE 4096

#ifndef DEBUG
# define DEBUG false
#endif


Request::Request() : state(READ_START_LINE), content_length(0), 
                     body_size(0), chunk_size(0), method(UNKNOWN), error_code(0){}

Request::Request(const Request& other) {
    *this = other;
}

Request& Request::operator=(const Request& other) {
    if (this != &other) {
        state = other.state;
        buffer = other.buffer;
        content_length = other.content_length;
        body_size = other.body_size;
        chunk_size = other.chunk_size;
        method = other.method;
        method_str = other.method_str;
        path = other.path;
        http_version = other.http_version;
        query_string = other.query_string;
        headers = other.headers;
        body_file_path = other.body_file_path;
    }
    return *this;
}

Request::~Request()
{
    if (body_file.is_open()) {
        body_file.close();
    }

	if (!body_file_path.empty()) {
        std::remove(body_file_path.c_str());
    }
}

void Request::append_to_buffer(const char *data, ssize_t size)
{
    if (size > 0) {
        buffer.append(data, size);
        parser(); 
    }
}

void Request::parser()
{
    if (state == ERROR || state == FINISHED) return;
    RequestState previous_state;

    try {
        do {
            previous_state = state;

            if (state == READ_START_LINE)
                parse_start_line();
            else if (state == READ_HEADERS)
                parse_headers();
            else if (state == READ_BODY)
                parse_plain_body();
            else if (state == READ_CHUNK_SIZE || state == READ_CHUNK_DATA)
                parse_chunked_body();
                
        } while (state != previous_state && state != FINISHED && state != ERROR);
    } 
    catch (const BadRequestException& e) {
        state = ERROR;
        error_code = 400;
    }
    catch (const NotImplementedException& e) {
        state = ERROR;
        error_code = 501;
    }
    catch (const LengthRequiredException& e) {
        state = ERROR;
        error_code = 411; 
    }
    catch (const std::exception& e) {
        state = ERROR;
        error_code = 500; 
    }
}


void Request::parse_start_line()
{
    size_t sp_pos = buffer.find("\r\n");
    if (sp_pos == std::string::npos)
        return;
    std::string first_line = buffer.substr(0, sp_pos);
    size_t first_sp, second_sp;
    first_sp = first_line.find_first_of(' ');
    second_sp = first_line.find_last_of(' ');

    if (first_line.find(' ', first_sp + 1) != second_sp)
        throw BadRequestException("Invalid request line");

    method_str = first_line.substr(0, first_sp);
    path = first_line.substr(first_sp + 1, second_sp - first_sp - 1);
    http_version = first_line.substr(second_sp + 1);
 
    size_t q = path.find('?');
    if (q != std::string::npos) {
        query_string = path.substr(q + 1);
        path = path.substr(0, q);
    } else {
        query_string = "";
    }

    if (method_str == "GET") method = GET;
    else if (method_str == "POST") method = POST;
    else if (method_str == "DELETE") method = DELETE;
    else throw NotImplementedException("Unsupported HTTP method: " + method_str);

    if (path.empty() || (http_version != "HTTP/1.1" && http_version != "HTTP/1.0"))
        throw BadRequestException("Invalid request line");

    buffer.erase(0, sp_pos + 2);   
    state = READ_HEADERS;
}

void Request::parse_headers()
{
    size_t pos = buffer.find("\r\n\r\n");
    if (pos == std::string::npos) return; 

    std::string raw_headers = buffer.substr(0, pos);
    buffer.erase(0, pos + 4);

    headers = Header(raw_headers); 

    if (headers.hasHeader("transfer-encoding")) {
        std::string value = headers.getHeader("transfer-encoding");
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);
        
        if (value == "chunked") {
            setup_body_file();
            state = READ_CHUNK_SIZE;
        } else {
            throw NotImplementedException("Transfer-Encoding not supported");
        }
    } 
    else if (headers.hasHeader("content-length")) {
        std::string value = headers.getHeader("content-length");      
        if (value.empty() || value.find_first_not_of("0123456789") != std::string::npos) {
            throw BadRequestException("Invalid Content-Length header");
        }

        content_length = std::strtol(value.c_str(), NULL, 10);
        if (content_length == 0) {
            state = FINISHED;
        } else {
            setup_body_file();
            state = READ_BODY;
        }
    } 

    else if (method == POST) {
        throw LengthRequiredException("POST request missing Content-Length header");
    } 
    else {
        state = FINISHED;
    }
}


void Request::parse_plain_body() {
    size_t remaining = content_length - body_size;
    size_t to_write = std::min(buffer.size(), remaining);

    if (to_write > 0) {
        body_file.write(buffer.data(), to_write);
        body_size += to_write;
        buffer.erase(0, to_write);
    }

    if (body_size == content_length) {
        body_file.close();
        state = FINISHED;
    }
}

void Request::parse_chunked_body()
{
    while (true) {
        if (state == READ_CHUNK_SIZE)
		{
            size_t pos = buffer.find("\r\n");
            if (pos == std::string::npos) return; 

            std::string hex_str = buffer.substr(0, pos);
            buffer.erase(0, pos + 2);

            chunk_size = std::strtol(hex_str.c_str(), NULL, 16);

            if (chunk_size == 0) {
                body_file.close();
                state = FINISHED;

                if (buffer.size() >= 2 && buffer.substr(0, 2) == "\r\n") {
                    buffer.erase(0, 2);
                }
                return;
            }
            state = READ_CHUNK_DATA;
        }
        else if (state == READ_CHUNK_DATA)
		{
            if (buffer.size() < chunk_size + 2) return;

            body_file.write(buffer.data(), chunk_size);
            body_size += chunk_size;
            
            buffer.erase(0, chunk_size + 2); 
            state = READ_CHUNK_SIZE; 
        }
    }
}


#include <sstream>
#include <stdexcept>

void Request::setup_body_file() {
    std::ostringstream oss;

    oss << "/tmp/webserv_body_" << this;  
    body_file_path = oss.str();

    body_file.open(body_file_path.c_str(), std::ios::out | std::ios::binary);
    
    if (!body_file.is_open()) {
        throw std::runtime_error("Cannot create temporary body file");
    }
}

//getters here

RequestState Request::get_state() const { return state; }
bool Request::is_finished() const { return state == FINISHED; }

int Request::get_error_code() const { return error_code; } // Needed for the Router

RequestMethod Request::get_method() const { return method; }
const std::string& Request::get_method_str() const { return method_str; }
const std::string& Request::get_path() const { return path; }
const std::string& Request::get_query_string() const { return query_string; }
const std::string& Request::get_http_version() const { return http_version; }

size_t Request::get_content_length() const { return content_length; }
size_t Request::get_body_size() const { return body_size; }

const std::string& Request::get_header(const std::string &name) const {
    return headers.getHeader(name);
}

const std::string& Request::get_body_file_path() const {
    return body_file_path;
}

const std::map<std::string, std::string>& Request::getHeaders() {
    return headers.getHeadersMap();
}
