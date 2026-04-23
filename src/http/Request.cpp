#include "Request.hpp"
#include <unistd.h>
#include <cstdlib>
#include <sys/socket.h>

#define BUFFER_SIZE 4096

Request::Request( void )
{
	_state = READ_START_LINE;
	_method = "";
	_path = "";
	_server_block = NULL;
	_headers = NULL;
	_pos = 0;
	_content_length = 0;
	_read_bytes = 0;
	_parse[READ_START_LINE] = &Request::extract_first_line;
	_parse[READ_HEADERS] = &Request::extract_headers;
	_parse[READ_PLAIN_BODY] = &Request::extract_plain_body;
	_parse[READ_CHUNK_BODY] = &Request::extract_chunked_body;
}

Request::~Request( void )
{
}

bool Request::is_finished( void )
{
	return (_state == FINISHED);
}

const std::string& Request::get_path( void ) const
{
	return _path;
}

const std::string& Request::get_method( void ) const
{
	return _method;
}

// void Request::set_server_block(ServerBlock *server_block)
// {
// 	_server_block = server_block;
// }

void Request::set_server_block(std::vector<ServerBlock *> server_blocks)
{
	std::string host = _headers->getHeader("host");
	for (size_t i = 0; i < server_blocks.size(); ++i) {
		for (size_t j = 0; j < server_blocks[i]->server_names.size(); j++) {
			if (server_blocks[i]->server_names[j] == host) {
				_server_block = server_blocks[i];
				return ;
			}
		}
	}

	throw BadRequestException("No server block found");
}

ServerBlock *Request::get_server_block( void ) const
{
	return _server_block;
}

void Request::read_request( int socket_fd, bool *closed )
{
	ssize_t size;
	char buf[4096];

	if (_state == READ_PLAIN_BODY) {
		size_t to_read = _content_length - _read_bytes;
		if (to_read > BUFFER_SIZE - 1)
			to_read = BUFFER_SIZE - 1;
		size = recv(socket_fd, buf, sizeof(buf), 0);
		_read_bytes += size;
	}
	else if (_state == READ_CHUNK_BODY) {
		size = recv(socket_fd, buf, _read_bytes, 0);
	}
	else {
		size = recv(socket_fd, buf, sizeof(buf), 0);
	}
	if (size == 0) {
		*closed = true;
		return ;
	}
	else if (size < 0)
		throw BadRequestException("Failed to read from socket");
	buf[size] = 0;
	std::cout << "Received data:\n" << buf << std::endl;
	append_to_buffer(buf);
	_parser();
}

void Request::append_to_buffer( const char *s )
{
	_buffer += s;
}

void Request::_parser( void )
{
	if (_state == FINISHED)
		   return ;
	if ((this->*_parse[_state])())
		_parser();
}

bool Request::extract_first_line( void )
{
	std::string all_method = "GETPOSTDELETE";
	size_t sp_pos = _buffer.find("\r\n");
	if (sp_pos == std::string::npos)
		return false;
	std::string first_line = _buffer.substr(0, sp_pos);
	size_t first_sp, second_sp;
	first_sp = first_line.find_first_of(' ');
	second_sp = first_line.find_last_of(' ');

	if (first_line.find(' ', first_sp + 1) != second_sp)
		throw BadRequestException("Invalid request line");

	_method = first_line.substr(0, first_sp);
	_path = first_line.substr(first_sp + 1, second_sp - first_sp - 1);
	_http_version = first_line.substr(second_sp + 1);

	if (all_method.find(_method) == std::string::npos || _path.empty() || _http_version != "HTTP/1.1")
		throw BadRequestException("Invalid request line");
	
	_buffer = _buffer.substr(sp_pos + 2);
	_pos = 0;
	_state = READ_HEADERS;
	return true;
}

bool Request::extract_headers( void )
{
	size_t sp_pos = _buffer.find("\r\n\r\n");
	if (sp_pos == std::string::npos)
		return (false);

	std::string header_str = _buffer.substr(_pos, sp_pos - _pos);
	_headers = new Header(header_str);
	_pos = sp_pos + 4;
	if (_headers->hasHeader("transfer-encoding")) {
		std::string value = _headers->getHeader("transfer-encoding");
		to_lower(value);
		if (value == "chunked")
			_state = READ_CHUNK_BODY;
		else
			throw NotEmplementedException("Transfer-Encoding not supported");
		_read_bytes = BUFFER_SIZE;
	}
	else if (_headers->hasHeader("content-length")) {
		std::string charset = "0123456789";
		std::string value = _headers->getHeader("content-length");
		if (value.empty() || has_other(value, charset))
			throw BadRequestException("Invalid Content-Length header");
		long n = std::strtol(value.c_str(), NULL, 10);
		_content_length = n;
		if (n == 0)
			_state = FINISHED;
		else
			_state = READ_PLAIN_BODY;
	}
	else
		_state = FINISHED;
	
	_buffer = _buffer.substr(_pos);
	_pos = 0;

	return (true);
}

bool Request::extract_plain_body( void )
{
	if (_buffer.length() != _content_length)
		return (false);
	_body = _buffer;
	_state = FINISHED;
	_pos = 0;
	_buffer.clear();
	return (true);
}

/**
	6\r\n
	Hello \r\n
	3\r\n
	42 \r\n
	8\r\n
	Network!\r\n
	0\r\n
	\r\n

1.	b = "6\r\nHello"	<= pos = 0	|	buffer
	n =	  1				<= 				del_pos


	5\r\nWorld\r\n
	     .
	7 - 4 = 3
 */

bool Request::extract_chunked_body( void )
{
	size_t n = _buffer.find("\r\n", _pos);
	if (n == std::string::npos) {
		if (_pos != 0) {
			_buffer = _buffer.substr(_pos);
			_pos = 0;
		}
		return (false);
	}
	std::string hex = _buffer.substr(_pos, n - _pos);
	std::string charset = "0123456789aAbBcCdDfFeE";
	if (has_other(hex, charset))
		throw BadRequestException("Invalid chunk size: [" + hex + "]");
	_read_bytes = std::strtol(hex.c_str(), NULL, 16) + 2;
	_pos += 2 + hex.size();
	if (_buffer.size() - _pos >= _read_bytes) {
		_body += _buffer.substr(_pos, _read_bytes);
		_pos += _read_bytes;
	}
	else {
		_body += _buffer.substr(_pos);
		_read_bytes -= _buffer.size() - _pos;
		_pos = _buffer.size();
	}
	if (hex == "0") {
		_state = FINISHED;
	}
	std::cout << "\n\nchunked body: " << _body << std::endl;
	return (true);
}