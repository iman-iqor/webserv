#include <iostream>
#include <cassert>
#include <cstring>
#include <map>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include "../CgiHandler.hpp"
#include "../Request.hpp"
#include "../../server/Client.hpp"
#include "../../server/Server.hpp"
#include "test_utils.hpp"



void fill_env_map(std::map< std::string, std::string > &env_map) {
    env_map["REQUEST_METHOD"] = "GET";
    env_map["SCRIPT_NAME"] = "./src/www/cgi-bin/test.py";
    env_map["QUERY_STRING"] = "name=John&age=30";
    env_map["HTTP_HOST"] = "localhost:8080";
    env_map["HTTP_USER_AGENT"] = "Mozilla/5.0";
    env_map["HTTP_COOKIE"] = "sessionid=abc123xyz; theme=dark; last_visit=2026-04-03";
}

int main(void) {
    std::string bin_path = "/usr/bin/python3";
    std::string cgi_path = "./src/www/cgi-bin/test.py";

    Client client(-1, -1);
    std::map< std::string, std::string > mock_env_map;

    fill_env_map(mock_env_map);


    MockCgiHandler::MockStart(&client,
        cgi_path,
        bin_path,
        mock_env_map);
}




CgiState_t *MockCgiHandler::MockStart(
	Client *client,
	const std::string &cgi_path,
	std::string &bin_path,
	const std::map< std::string, std::string > &env_map
) {
	CgiState_t *cgi_state = new CgiState_t;
    if (!cgi_state) {
		throw std::runtime_error("Failed to allocate memory for CGI state");
	}
	cgi_state->fdo[0] = -1;
	cgi_state->fdo[1] = -1;
	cgi_state->fdi[0] = -1;
	cgi_state->fdi[1] = -1;
	cgi_state->pid = -1;
	cgi_state->req_w_fd = -1;
	cgi_state->res_r_fd = -1;
	cgi_state->cgi_output = "";
	cgi_state->body_sent = 0;
	cgi_state->ready_to_send = false;
	bool is_post_with_body = (client->request.get_method() == "POST" && client->request.get_content_length() > 0);

	open_pipe(cgi_state, cgi_state->fdo); // Always open the output pipe for CGI response
	if (is_post_with_body) {
		open_pipe(cgi_state, cgi_state->fdi);
	}

	pid_t pid = fork();
	if (pid < 0) {
		close_pipes(cgi_state);
		throw std::runtime_error("fork error");
	}

	else if (pid == 0) { // Child process
		int null_fd = open("/dev/null", O_RDONLY);
		if (null_fd == -1) {
			close_pipes(cgi_state);
			throw std::runtime_error("Failed to open /dev/null");
		}
		if (is_post_with_body) {
			dup2(cgi_state->fdi[0], STDIN_FILENO); // Redirect CGI child's stdin to read end of request pipe
			dup2(cgi_state->fdo[1], STDOUT_FILENO); // Redirect CGI child's stdout to write end of response pipe
			dup2(null_fd, STDERR_FILENO); // Redirect CGI child's stderr to /dev/null if there's no request body
			close(null_fd);
		} else {
			dup2(cgi_state->fdo[1], STDOUT_FILENO); // Redirect CGI child's stdout to write end of response pipe
			dup2(null_fd, STDERR_FILENO); // Redirect CGI child's stderr to /dev/null if there's no request body
			close(null_fd);
		}

		char **envp = CgiHandler::build_envp(env_map);
		char **argv = CgiHandler::build_args(cgi_path, bin_path);
		execve(bin_path.c_str(), argv, envp);
		CgiHandler::free_envp(envp);
		CgiHandler::free_args(argv);
		// WARNING: more cleaning needed
		exit(1); // If exec fails
	}

	// TODO: Check if all fd are closed
	// parent process
	if (is_post_with_body) {
		close(cgi_state->fdi[0]); // close read end of request pipe in parent
	}
	close(cgi_state->fdo[1]); // close write end of response pipe in parent

    ssize_t bytes_read;
    char buffer[10240];

    bytes_read = read(cgi_state->fdo[0], buffer, sizeof(buffer) - 1);
    buffer[bytes_read] = '\0';

    write(STDOUT_FILENO, buffer, bytes_read);
    int status;
    waitpid(cgi_state->pid, &status, 0); // Wait for child to finish

    printf("CGI child process exited with status %d\n", WEXITSTATUS(status));
	return cgi_state;
}