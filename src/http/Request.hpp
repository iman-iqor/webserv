#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <unistd.h>
#include <fstream>
#include "Header.hpp"
#include "Exceptions.hpp"
#include "../config/Config.hpp"

#define BODY_LIMIT 8192 // 8KB

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

enum ReadMethod {
	READ_TO_MEM,
	READ_TO_FIL
};

class Request {
    std::string _method;
	std::string _path;
    std::string _http_version;
    std::string _body;
	std::string _raw_bytes;
    std::string _buffer;
	std::fstream _outfile;
	size_t _body_size;
    size_t _pos;
	size_t _content_length;
	size_t _read_bytes;
	Header *_headers;
	RequestState _state;
	Location *_location;
	ReadMethod _read_method;
	bool _body_is_set;
    bool (Request::*_parse[4])( void );
	std::string _filename;
	ServerBlock *_server_block;
	std::map<std::string, std::string> _params;
	std::string _query_string;
	
public:
	Request( void );
	~Request( void );
    void _parser( void );

	ssize_t read_to_file( const char *buffer, ssize_t size );
	void append_request( const char *s, ssize_t size );
	bool extract_first_line( void );
	bool extract_headers( void );
	bool extract_plain_body( void );
	bool extract_chunked_body( void );
    bool is_finished( void );
	// add a method to start the save in file process, create a file and save fd
	void start_save_to_file( void );
	void close_outfile( void );

	/* Getters */
	std::string &get_filename(void);
	size_t get_content_length( void ) const;
	const std::string& get_path( void ) const;
	const std::string& get_method( void ) const;
	const std::string& getHeader(const std::string &name) const;
	std::string& get_body(void);
	const std::string& get_http_version(void) const;
	RequestState get_state( void ) const ;
	std::map<std::string, std::string> &getHeaders();
	std::string &get_query_string(void);

	/* Setters */
	void set_filename(const std::string &filename);
	ServerBlock *getServerBlock() const;
	void setServerBlock(ServerBlock *server_block);

};

#endif // REQUEST_HPP