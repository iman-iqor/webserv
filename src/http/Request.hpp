#ifndef REQUEST_HPP
#define REQUEST_HPP

#include "Header.hpp"
#include "Exceptions.hpp"
#include "../config/Config.hpp"

enum RequestState {
	READ_START_LINE,	// Parse first line: "METHOD SP TARGET SP HTTP/VERSION\r\n"
	READ_HEADERS,		// Parse headers until empty line: "\r\n\r\n"
	READ_PLAIN_BODY,	// Read body bytes based on Content-Length
	READ_CHUNK_BODY,	// Read chunked body until the final zero-size chunk is parsed
	FINISHED			// Request fully parsed and ready for routing/handling
};

enum RequestMethod {
    GET,
    POST,
    DELETE
};

class Request {
	RequestState _state;
    std::string _method;
	std::string _path;
    std::string _http_version;
    std::string _body;
	ServerBlock* _server_block;

    size_t _pos;
	size_t _content_length;
	size_t _read_bytes;
    std::string _buffer;
	Header *_headers;
	std::string _raw_bytes;

    bool (Request::*_parse[4])(void);

	
public:
	Request( void );
	~Request( void );
    void _parser( void );

    // void append_to_buffer(const std::string& data);
    void append_to_buffer(const char *s);

	bool extract_first_line( void );
	bool extract_headers( void );
	bool extract_plain_body( void );
	bool extract_chunked_body( void );

    bool is_finished( void );
	void validate( void );
	const std::string& get_path( void ) const;
	const std::string& get_method( void ) const;
	void set_server_block(std::vector<ServerBlock *> &server_blocks);
	ServerBlock *get_server_block( void ) const;

};

#endif // REQUEST_HPP