#include "CgiHandler.hpp"


char **CgiHandler::build_envp(const std::map< std::string, std::string > &env_map) {
	char **envp = new char*[env_map.size() + 1];
	int i = 0;
	std::map<std::string, std::string>::const_iterator it = env_map.begin();
	for (; it != env_map.end(); it++) {
		std::string env_entry = it->first + "=" + it->second;
		envp[i] = new char[env_entry.size() + 1];
		std::strcpy(envp[i], env_entry.c_str());
		i++;
	}
	envp[i] = NULL;
	return envp;
}

void CgiHandler::free_envp(char **envp) {
	for (int i = 0; envp[i] != NULL; i++) {
		delete[] envp[i];
	}
	delete[] envp;
}

void CgiHandler::set_non_blocking(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1) {
		throw std::runtime_error("Failed to get file flags");
	}
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
		throw std::runtime_error("Failed to set non-blocking mode");
	}
}

void close_pipes(CgiState_t *cgi_state) {
	if (cgi_state->fdi[0] != -1) close(cgi_state->fdi[0]);
	if (cgi_state->fdi[1] != -1) close(cgi_state->fdi[1]);
	if (cgi_state->fdo[0] != -1) close(cgi_state->fdo[0]);
	if (cgi_state->fdo[1] != -1) close(cgi_state->fdo[1]);
}

char **CgiHandler::build_args(const std::string &cgi_path, const std::string &bin_path) {
	char **argv = new char*[3];
	argv[0] = new char[bin_path.size() + 1];
	std::strcpy(argv[0], bin_path.c_str());
	argv[1] = new char[cgi_path.size() + 1];
	std::strcpy(argv[1], cgi_path.c_str());
	argv[2] = NULL;
	return argv;
}

void CgiHandler::free_args(char **argv) {
	for (int i = 0; argv[i] != NULL; i++) {
		delete[] argv[i];
	}
	delete[] argv;
}

void open_pipe(CgiState_t *cgi_state, int fds[2])
{
	if (pipe(fds) == -1) {
		close_pipes(cgi_state);
		throw std::runtime_error("pipe error");
	}
}

CgiState_t *CgiHandler::start(
	Client *client,
	const std::string &cgi_path,
	std::string &bin_path,
	int epoll_fd,
	const std::map< std::string, std::string > &env_map
) {
	CgiState_t *cgi_state = new CgiState_t;
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

	if (!cgi_state) {
		throw std::runtime_error("Failed to allocate memory for CGI state");
	}
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

		char **envp = build_envp(env_map);
		char **argv = build_args(cgi_path, bin_path);
		execve(bin_path.c_str(), argv, envp);
		free_envp(envp);
		free_args(argv);
		// WARNING: more cleaning needed
		exit(1); // If exec fails
	}

	// TODO: Check if all fd are closed
	// parent process
	if (is_post_with_body) {
		close(cgi_state->fdi[0]); // close read end of request pipe in parent
		set_non_blocking(cgi_state->fdi[1]);
	} else {
	}
	close(cgi_state->fdo[1]); // close write end of response pipe in parent

	set_non_blocking(cgi_state->fdo[0]);

	// Add fdo[0] to epoll for monitoring CGI output
	struct epoll_event event;
	event.events = EPOLLIN;
	EpollData* data = new EpollData;
	data->fd = cgi_state->fdo[0];
	data->type = CGI_PIPE;
	data->client = client;
	event.data.ptr = data;

	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cgi_state->fdo[0], &event) == -1) {
		if (is_post_with_body && cgi_state->fdi[1] != -1) close(cgi_state->fdi[1]);
		close(cgi_state->fdo[0]);
		kill(pid, SIGKILL);
		delete data;
		throw std::runtime_error("epoll_ctl error");
	}

	if (is_post_with_body) {
		// If there's a request body that hasn't been fully sent to the CGI child yet, we should also add the write end of the request pipe to epoll for monitoring when it's ready to send more data.
		set_non_blocking(cgi_state->fdi[1]);
		struct epoll_event req_event;
		req_event.events = EPOLLOUT;
		EpollData* req_data = new EpollData;
		req_data->fd = cgi_state->fdi[1];
		req_data->type = CGI_PIPE;
		req_data->client = client;
		req_event.data.ptr = req_data;

		if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cgi_state->fdi[1], &req_event) == -1) {
			if (cgi_state->fdi[1] != -1) close(cgi_state->fdi[1]);
			close(cgi_state->fdo[0]);
			kill(pid, SIGKILL);
			delete data;
			delete req_data;
			throw std::runtime_error("epoll_ctl error");
		}
	}

	// Update cgi_state with the fork result and return
	cgi_state->pid = pid;
	cgi_state->req_w_fd = cgi_state->fdi[1];
	cgi_state->res_r_fd = cgi_state->fdo[0];
	return cgi_state;
}