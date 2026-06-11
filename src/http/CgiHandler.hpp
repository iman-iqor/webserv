#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include <string>
#include <cstring>
#include <map>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdexcept>
#include "../server/Server.hpp"


class Client; // Forward declaration to avoid circular includes
/**
 * This struct holds the state of a CGI execution for a particular client. It includes:
 * - `pid`: The process ID of the CGI child process, which is needed to wait for the child to finish and to kill it if the client disconnects.
 * - `req_w_fd`: The file descriptor used to write the request body to the CGI child's standard input (stdin).
 * - `res_r_fd`: The file descriptor used to read the CGI child's standard output (stdout) for the response that will be sent back to the client.
 */
typedef struct CgiState_s {
	int fdo[2];	// pipe for CGI output (child writes to fdo[1], parent reads from fdo[0])
	pid_t pid;		// process ID of the CGI child
	int res_r_fd;	// fd to read CGI child's stdout for response
	std::string cgi_output; // buffer to hold CGI output until child exits (non-blocking)
	size_t body_sent; // track how many bytes of the request body have been sent to the CGI child
	size_t output_sent; // track how many bytes of the CGI output have been sent to the client
	bool ready_to_send; // flag to indicate whether the CGI child has exited and the
} CgiState_t;


class CgiHandler {
public:
	static char **build_envp(const std::map< std::string, std::string > &env_map);
	static void free_envp(char **envp);

    static char **build_args(const std::string &cgi_path, const std::string &bin_path);
    static void free_args(char **argv);

	static void set_non_blocking(int fd);

	static CgiState_t *start(
		Client *client,
		const std::string &cgi_path,
		const std::string &bin_path,
		int epoll_fd,
		const std::map< std::string, std::string > &env_map
	);
};

void close_pipes(CgiState_t *cgi_state);
void open_pipe(CgiState_t *cgi_state, int fds[2]);

#endif // CGIHANDLER_HPP