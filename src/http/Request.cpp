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

Request::Request( void )
{
	_read_method = READ_TO_MEM;
	_state = READ_START_LINE;
	_method = "";
	_path = "";
	_headers = NULL;
	_pos = 0;
	_body_size = 0;
	_content_length = 0;
	_location = NULL;
	_read_bytes = 0;
	_body_is_set = false;
	_parse[READ_START_LINE] = &Request::extract_first_line;
	_parse[READ_HEADERS] = &Request::extract_headers;
	_parse[READ_PLAIN_BODY] = &Request::extract_plain_body;
	_parse[READ_CHUNK_BODY] = &Request::extract_chunked_body;
	_read[READ_TO_MEM] = &Request::read_to_mem;
	_read[READ_TO_FIL] = &Request::read_to_file;
	if (DEBUG) std::cout << BOLD_CYAN << "[REQUEST]" << RESET << " Parser initialized" << std::endl;
}

Request::~Request( void )
{
	delete _headers;
	if (_outfile.is_open())
		_outfile.close();
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

ssize_t Request::read_to_mem( const char *s, ssize_t size )
{
	if (DEBUG) std::cout << CYAN << "[REQUEST]" << RESET << " reading " << size << " bytes into buffer" << std::endl;;
	if (_body_size + static_cast<size_t>(size) > _content_length)
		size = _content_length - _body_size;
	_body.append(s, size);
	_body_size += size;
	return size;
}

ssize_t Request::read_to_file( const char *s, ssize_t size )
{
	if (DEBUG) std::cout << CYAN << "[REQUEST]" << RESET << " reading " << size << " bytes into file" << std::endl;
	if (!_outfile.is_open())
		throw InternalServerErrorException("Temporary file for request body is not open");
	if (_body_size + static_cast<size_t>(size) > _content_length)
		size = _content_length - _body_size;
	_outfile.write(s, size);
	_body_size += size;
	return size;
}

void Request::append_request( const char *s, ssize_t size )
{
	if (DEBUG) std::cout << CYAN << "[REQUEST]" << RESET << " Received bytes, buffer size=" << size << std::endl;
	_buffer.append(s, size);
	_parser();
}

void Request::_parser( void )
{
	if (_state == FINISHED)
		   return ;
	if (DEBUG) std::cout << CYAN << "[REQUEST]" << RESET << " Parsing state=" << _state << std::endl;
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
	if (DEBUG) std::cout << BOLD_CYAN << "[REQUEST]" << RESET << " Start line => method=" << _method << " path=" << _path << " version=" << _http_version << std::endl;

	if (!method_is_valid(_method))
		throw NotImplementedException("Unsupported HTTP method: " + _method);
	if (_path.empty() || (_http_version != "HTTP/1.1" && _http_version != "HTTP/1.0"))
		throw BadRequestException("Invalid request line");
	
	_buffer = _buffer.substr(sp_pos + 2);
	_pos = 0;
	_state = READ_HEADERS;
	if (DEBUG) std::cout << CYAN << "[REQUEST]" << RESET << " Transition to READ_HEADERS" << std::endl;
	return true;
}

const std::string& Request::get_http_version(void) const
{
	return _http_version;
}

void Request::start_save_to_file( void )
{
	if (DEBUG) std::cout << BOLD_CYAN << "[REQUEST]" << RESET << " creating temporary file for request body" << std::endl;
	_read_method = READ_TO_FIL;
	char buf[17];
	std::ifstream file("/dev/urandom");
	if (!file.is_open())
		throw InternalServerErrorException("Failed to open /dev/urandom for generating temporary filename");
	file.read(buf, 16);
	buf[16] = '\0';
	file.close();
	filename = "/tmp/request_" + std::string(buf) + ".tmp";
	if (DEBUG) std::cout << BOLD_CYAN << "[REQUEST]" << RESET << " saving request body to file: " << filename << std::endl;
	// use file.seekg(0, std::ios::beg); to reset the file pointer to the beginning of the file before reading again if needed, since we have already read 16 bytes from /dev/urandom to generate the filename. This ensures that we can read from /dev/urandom again if we need to generate another filename in the future without any issues.
	_outfile.open(filename.c_str(), std::ios::binary | std::ios::out | std::ios::in | std::ios::trunc);
	if (!_outfile.is_open())
		throw InternalServerErrorException("Failed to create temporary file for request body");
	// unlink(filename.c_str());
}

bool Request::extract_headers( void )
{
	size_t sp_pos = _buffer.find("\r\n\r\n");
	if (sp_pos == std::string::npos)
		return (false);

	std::string header_str = _buffer.substr(_pos, sp_pos - _pos);
	_headers = new Header(header_str);
	if (DEBUG) std::cout << BOLD_MAGENTA << "[REQUEST]" << RESET << " Headers parsed successfully" << std::endl;
	_pos = sp_pos + 4;
	if (_headers->hasHeader("transfer-encoding")) {
		std::string value = _headers->getHeader("transfer-encoding");
		to_lower(value);
		if (value == "chunked")
			_state = READ_CHUNK_BODY;
		else
			throw NotImplementedException("Transfer-Encoding not supported");
		_read_bytes = BUFFER_SIZE;
		if (DEBUG) std::cout << MAGENTA << "[REQUEST]" << RESET << " Using chunked body parser" << std::endl;
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
		{
			if (n > BODY_LIMIT)
				start_save_to_file();
			_state = READ_PLAIN_BODY;
		}
		if (DEBUG) std::cout << MAGENTA << "[REQUEST]" << RESET << " Content-Length=" << _content_length << std::endl;
	}
	else if (_method == "POST")
		throw LengthRequiredException("POST request missing Content-Length header");
	else
		_state = FINISHED;
	if (DEBUG && _state == FINISHED) std::cout << BOLD_GREEN << "[REQUEST]" << RESET << " Request completed after headers" << std::endl;
	
	_buffer = _buffer.substr(_pos);
	_pos = 0;

	return (true);
}

RequestState Request::get_state( void ) const
{
	return _state;
}

bool Request::extract_plain_body( void )
{
	(this->*_read[_read_method])(_buffer.c_str(), _buffer.size());
	if (_body_size < _content_length)
		return (false);
	_state = FINISHED;
	if (DEBUG) std::cout << BOLD_GREEN << "[REQUEST]" << RESET << " Plain body parsed, bytes=" << _body.size() << std::endl;
	_pos = 0;
	_buffer.clear();
	if (_body_size == _content_length)
		_state = FINISHED;
	else
		throw BadRequestException("Body size does not match Content-Length");
	return (true);
}

bool Request::extract_chunked_body( void )
{
	std::string dechunked_body;

	while (true) {
		size_t chunk_start = _pos;
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
			std::string chunk = _buffer.substr(_pos, chunk_size);
			(this->*_read[_read_method])(chunk.c_str(), chunk.size());
			_pos += chunk_size + 2;
		}
		else {
			_buffer = _buffer.substr(chunk_start);
			_pos = 0;
			return (false);
		}
		if (DEBUG) std::cout << CYAN << "[REQUEST]" << RESET << " Chunk fragment parsed, current size=" << _body.size() << std::endl;
	}
	return (true);
}

size_t Request::get_content_length( void ) const
{
	return _content_length;
}

//imane added this
const std::string& Request::getHeader(const std::string &name) const
{
    return _headers->getHeader(name);
}

std::string& Request::get_body(void)
{
	if (_read_method == READ_TO_FIL && _body_is_set == false) {
		if (!_outfile.is_open())
			throw InternalServerErrorException("Temporary file for request body is not open");
		_outfile.clear();
		_outfile.seekg(0, std::ios::beg);
		char buf[BUFFER_SIZE];
		while (_outfile.read(buf, sizeof(buf))) {
			_body.append(buf, _outfile.gcount());
		}
		if (_outfile.gcount() > 0) {
			_body.append(buf, _outfile.gcount());
		}
		_body_is_set = true;
	}
    return _body;
}

std::map<std::string, std::string> &Request::getHeaders()
{
	return _headers->getHeaders();
}
