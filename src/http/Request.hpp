#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <unistd.h>
#include <fstream>
#include "Header.hpp"
#include "Exceptions.hpp"
#include "../config/Config.hpp"

#define BODY_LIMIT 8192 

enum RequestState {
	READ_START_LINE,	
	READ_HEADERS,	
	READ_BODY,	
	READ_CHUNK_DATA,
	READ_CHUNK_SIZE,	
	FINISHED,
	ERROR			
};

enum RequestMethod {
    GET,
    POST,
    DELETE,
	UNKNOWN
};

enum ReadMethod {
	READ_TO_MEM,
	READ_TO_FIL
};

class Request
{
	private:
		RequestState state;

		std::string buffer;
		size_t content_length;
		size_t body_size;
		size_t chunk_size;


		RequestMethod 	method;
		std::string method_str;
		std::string path;
		std::string http_version;
		std::string query_string;

		std::string body_file_path;
		std::ofstream body_file;

		int error_code;

		Header headers;
		void parse_start_line();
		void parse_headers();
		void parse_plain_body();
		void parse_chunked_body();
		std::string trim(const std::string& str);
    	void setup_body_file();

	public:
		Request();
		Request(const Request& other);
    	Request& operator=(const Request& other);
		~Request();
		void append_to_buffer(const char *data, ssize_t size);
		RequestState get_state() const;

    	bool is_finished() const;
		RequestMethod get_method() const;

		const std::string& get_method_str() const;
		const std::string& get_body();
		const std::string& get_query_string() const;
		const std::string& get_http_version() const;
		const std::string& get_body_file_path() const;
		size_t get_body_size() const;
		size_t get_content_length( void ) const;
		const std::string& get_path( void ) const;
		const std::string& getHeader(const std::string &name) const;
		int	get_error_code() const;
		
		void parser(); 
		const std::string& get_header(const std::string &name) const;
		const std::map<std::string, std::string> &getHeaders();
};

#endif