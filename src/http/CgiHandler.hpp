#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include <string>
#include <cstring>
#include <map>
#include <sys/types.h>
#include "Client.hpp"

/**
 * This struct holds the state of a CGI execution for a particular client. It includes:
 * - `pid`: The process ID of the CGI child process, which is needed to wait for the child to finish and to kill it if the client disconnects.
 * - `req_w_fd`: The file descriptor used to write the request body to the CGI child's standard input (stdin).
 * - `res_r_fd`: The file descriptor used to read the CGI child's standard output (stdout) for the response that will be sent back to the client.
 */
typedef struct CgiState_s {
	pid_t pid;		// process ID of the CGI child
	int req_w_fd;	// fd to write request body to CGI child's stdin
	int res_r_fd;	// fd to read CGI child's stdout for response
	std::string cgi_output; // buffer to hold CGI output until child exits (non-blocking)
	size_t body_sent; // track how many bytes of the request body have been sent to the CGI child
	bool ready_to_send; // flag to indicate whether the CGI child has exited and the
} CgiState_t;


class CgiHandler {
public:
	static char **build_envp(const std::map< std::string, std::string > &env_map);

	static void free_envp(char **envp);

	static void set_non_blocking(int fd);

	static void start(
		Client *client,
		const std::string &cgi_path,
		std::string &bin_path,
		int epoll_fd,
		const std::map< std::string, std::string > &env_map
	);
};



#endif // CGIHANDLER_HPP