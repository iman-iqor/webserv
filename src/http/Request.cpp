#include "Request.hpp"
#include <unistd.h>
#include <cstdlib>

#define BUFFER_SIZE 4096



Request::Request( void )
{
	_state = READ_START_LINE;
	_method = "";
	_path = "";
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

void Request::read_request( int socket_fd )
{
	ssize_t size;
	char buf[4096];

	if (_state == READ_PLAIN_BODY)
	{
		size_t to_read = _content_length - _read_bytes;
		if (to_read > BUFFER_SIZE - 1)
			to_read = BUFFER_SIZE - 1;
		size = read(socket_fd, buf, to_read);
		if (size < 0)
			throw BadRequestException();
		if (size == 0)
			return ;
		buf[size] = 0;
		append_to_buffer(buf);
		_read_bytes += size;
	}
	else
	{
		size = read(socket_fd, buf, BUFFER_SIZE - 1);
		if (size < 0)
		throw BadRequestException();
		if (size == 0)
			return ;
		buf[size] = 0;	
		append_to_buffer(buf);
	}
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
		throw BadRequestException();

	_method = first_line.substr(0, first_sp);
	_path = first_line.substr(first_sp + 1, second_sp - first_sp - 1);
	_http_version = first_line.substr(second_sp + 1);

	if (all_method.find(_method) == std::string::npos || _path.empty() || _http_version != "HTTP/1.1")
		throw BadRequestException();
	
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
			throw NotEmplementedException();
	}
	else if (_headers->hasHeader("content-length")) {
		std::string charset = "0123456789";
		std::string value = _headers->getHeader("content-length");
		if (value.empty() || has_other(value, charset))
			throw BadRequestException();
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

bool Request::extract_chunked_body( void )
{
	return false;
}