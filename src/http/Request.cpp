#include "Request.hpp"
#include <unistd.h>
#include <cstdlib>
#include <sys/socket.h>
#include <algorithm>

#define BUFFER_SIZE 4096

#ifndef VERBOS
# define VERBOS false
#endif

Request::Request( void )
{
	_state = READ_START_LINE;
	_method = "";
	_path = "";
	_server_block = NULL;
	_headers = NULL;
	_pos = 0;
	_content_length = 0;
	_location = NULL;
	_read_bytes = 0;
	_parse[READ_START_LINE] = &Request::extract_first_line;
	_parse[READ_HEADERS] = &Request::extract_headers;
	_parse[READ_PLAIN_BODY] = &Request::extract_plain_body;
	_parse[READ_CHUNK_BODY] = &Request::extract_chunked_body;
	if (VERBOS) std::cout << BOLD_CYAN << "[REQUEST]" << RESET << " Parser initialized" << std::endl;
}

Request::~Request( void )
{
	delete _headers;
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

void Request::set_server_block(std::vector<ServerBlock *> *server_blocks)
{
	(void) server_blocks;
	std::string host = _headers->getHeader("host");
	if (VERBOS) std::cout << CYAN << "[REQUEST]" << RESET << " Looking for server block host=" << host << std::endl;
	for (size_t i = 0; i < sv_blocks->size(); ++i) {
		std::vector<std::string> &names = (*sv_blocks)[i]->server_names;
		for (size_t j = 0; j < names.size(); j++) {
			if (names[j] == host) {
				if (VERBOS) std::cout << BOLD_GREEN << "[REQUEST]" << RESET << " Server block found: " << GREEN << host << RESET << std::endl;
				_server_block = (*sv_blocks)[i];
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

void Request::append_to_buffer( const char *s )
{
	_buffer += s;
	if (VERBOS) std::cout << CYAN << "[REQUEST]" << RESET << " Received bytes, buffer size=" << _buffer.size() << std::endl;
	_parser();
}

void Request::_parser( void )
{
	if (_state == FINISHED)
		   return ;
	if (VERBOS) std::cout << CYAN << "[REQUEST]" << RESET << " Parsing state=" << _state << std::endl;
	if ((this->*_parse[_state])())
		_parser();
}

bool Request::extract_first_line( void )
{
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
	if (VERBOS) std::cout << BOLD_CYAN << "[REQUEST]" << RESET << " Start line => method=" << _method << " path=" << _path << " version=" << _http_version << std::endl;

	if (!method_is_valid(_method))
		throw NotImplementedException("Unsupported HTTP method: " + _method);
	if (_path.empty() || (_http_version != "HTTP/1.1" && _http_version != "HTTP/1.0"))
		throw BadRequestException("Invalid request line");
	
	_buffer = _buffer.substr(sp_pos + 2);
	_pos = 0;
	_state = READ_HEADERS;
	if (VERBOS) std::cout << CYAN << "[REQUEST]" << RESET << " Transition to READ_HEADERS" << std::endl;
	return true;
}

bool Request::extract_headers( void )
{
	size_t sp_pos = _buffer.find("\r\n\r\n");
	if (sp_pos == std::string::npos)
		return (false);

	std::string header_str = _buffer.substr(_pos, sp_pos - _pos);
	_headers = new Header(header_str);

	// TODO: pre validate the Routing HERE <<<<<<<<
	// in here we will implement the Routing logic
	// which is to find the matching server block and location block for the request path
	// and also to check if the request method is supported by the location block.

	if (VERBOS) std::cout << BOLD_MAGENTA << "[REQUEST]" << RESET << " Headers parsed successfully" << std::endl;
	_pos = sp_pos + 4;
	if (_headers->hasHeader("transfer-encoding")) {
		std::string value = _headers->getHeader("transfer-encoding");
		to_lower(value);
		if (value == "chunked")
			_state = READ_CHUNK_BODY;
		else
			throw NotImplementedException("Transfer-Encoding not supported");
		_read_bytes = BUFFER_SIZE;
		if (VERBOS) std::cout << MAGENTA << "[REQUEST]" << RESET << " Using chunked body parser" << std::endl;
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
		if (VERBOS) std::cout << MAGENTA << "[REQUEST]" << RESET << " Content-Length=" << _content_length << std::endl;
	}
	else if (_method == "POST")
		throw LengthRequiredException("POST request missing Content-Length header");
	else
		_state = FINISHED;
	if (VERBOS && _state == FINISHED) std::cout << BOLD_GREEN << "[REQUEST]" << RESET << " Request completed after headers" << std::endl;
	
	_buffer = _buffer.substr(_pos);
	_pos = 0;

	set_server_block(sv_blocks);
	pre_validate();

	return (true);
}

bool Request::is_method_supported(const std::string& method) const
{
	return (std::find(_location->methods.begin(), _location->methods.end(), method) != _location->methods.end());
}

void Request::pre_validate( void )
{
	_location = find_location(_server_block, _path);
	if (_location == NULL)
		throw NotFoundException("No matching location found for path: " + _path);
	if (!is_method_supported(_method))
		throw MethodNotAllowedException("Method not supported: " + _method);
}

RequestState Request::get_state( void ) const
{
	return _state;
}

// void Request::validate( void )
// {
// 	if (_state != FINISHED)
// 		throw BadRequestException("Request is not fully parsed");
// 	if (_body.size() != _content_length)
// 		throw BadRequestException("Body size does not match Content-Length");
// 	if (VERBOS) std::cout << BOLD_GREEN << "[REQUEST]" << RESET << " Request validation successful" << std::endl;
// }

bool Request::extract_plain_body( void )
{
	if (_buffer.length() < _content_length)
		return (false);
	_body = _buffer.substr(0, _content_length);
	_state = FINISHED;
	if (VERBOS) std::cout << BOLD_GREEN << "[REQUEST]" << RESET << " Plain body parsed, bytes=" << _body.size() << std::endl;
	_pos = 0;
	_buffer.clear();
	if (_body.size() == _content_length)
		_state = FINISHED;
	else
		throw BadRequestException("Body size does not match Content-Length");
	return (true);
}

bool Request::extract_chunked_body( void )
{
	while (true) {
		size_t n = _buffer.find("\r\n", _pos);
		if (n == std::string::npos) {			// if i can't find the next line SEP, return and wait for more data to arrive
			if (_pos != 0) { 					// if i have already parsed some chunk, but the next chunk size line is not complete, keep the unprocessed part in the buffer for the next parsing round
				_buffer = _buffer.substr(_pos);	// keep the unprocessed part of the buffer starting from the current position, which may contain a partial chunk size line or chunk data that has not been fully parsed yet. This allows the parser to continue processing the remaining data when more bytes arrive in subsequent reads.
				_pos = 0;						// reset the position to the beginning of the new buffer
			}
			return (false);
		}
		std::string hex = _buffer.substr(_pos, n - _pos); // find the next chunk size line, which is terminated by "\r\n". The chunk size is represented as a hexadecimal string, so we extract the substring from the current position to the position of the next "\r\n" to get the chunk size in hex format.
		if (hex.find_first_not_of("0123456789aAbBcCdDeEfF") != std::string::npos)
			throw BadRequestException("Invalid chunk size: [" + hex + "]");
		_pos += 2 + hex.size();
		unsigned int chunk_size = std::strtol(hex.c_str(), NULL, 16);
		if (chunk_size == 0 && _buffer.find("\r\n\r\n", _pos) == std::string::npos) { // if the chunk size is 0, it indicates the last chunk, but we should also make sure that the final "\r\n\r\n" after the last chunk is received to mark the end of the chunked body. If we haven't received the final "\r\n\r\n" yet, we should wait for more data to arrive instead of marking the request as finished.
			_state = FINISHED;
			break;
		}
		if (_buffer.size() - _pos >= (size_t)chunk_size) {
			_body += _buffer.substr(_pos, chunk_size);
			_pos += chunk_size + 2;
		}
		else {
			_body += _buffer.substr(_pos);
			chunk_size -= _buffer.size() - _pos;
			_pos = _buffer.size();
		}
		if (VERBOS) std::cout << CYAN << "[REQUEST]" << RESET << " Chunk fragment parsed, current size=" << _body.size() << std::endl;
	}
	return (true);
}