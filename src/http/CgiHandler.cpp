#include "CgiHandler.hpp"
#include "../../webserv.h"


char **CgiHandler::build_envp(const std::map< std::string, std::string > &env_map) {
	if (DEBUG) std::cout << GREEN << "Building envp for CGI execution..." << RESET << std::endl;
	char **envp = new char*[env_map.size() + 1];
	int i = 0;
	std::map<std::string, std::string>::const_iterator it = env_map.begin();
	for (; it != env_map.end(); it++) {
		std::string env_entry = it->first + "=" + it->second;
		envp[i] = new char[env_entry.size() + 1];
		std::strcpy(envp[i], env_entry.c_str());
		if (DEBUG) std::cout << "  " << envp[i] << std::endl; // Debug print of each environment variable
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
	if (DEBUG) std::cout << GREEN << "Setting fd " << fd << " to non-blocking mode..." << RESET << std::endl;
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
	if (DEBUG) std::cout << GREEN << "Building argv for CGI execution..." << RESET << std::endl;
	char **argv = new char*[3];
	argv[0] = new char[bin_path.size() + 1];
	if (DEBUG) std::cout << "  argv[0] = " << bin_path << std::endl; // Debug print of argv[0]
	std::strcpy(argv[0], bin_path.c_str());
	argv[1] = new char[cgi_path.size() + 1];
	if (DEBUG) std::cout << "  argv[1] = " << cgi_path << std::endl; // Debug print of argv[1]
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
	if (DEBUG) std::cout << GREEN << "Opening pipe..." << RESET << std::endl;
	if (pipe(fds) == -1) {
		close_pipes(cgi_state);
		throw std::runtime_error("pipe error");
	}
}

CgiState_t *CgiHandler::start(
	Client *client,
	const std::string &cgi_path,
	const std::string &bin_path,
	int epoll_fd,
	const std::map< std::string, std::string > &env_map
) {
	if (DEBUG) {
		std::cout << GREEN << "Starting CGI execution for client..." << RESET << std::endl;
		std::cout << "  CGI path: " << cgi_path << std::endl;
		std::cout << "  Binary path: " << bin_path << std::endl;
	}

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
	if (DEBUG) std::cout << "  CGI request method: " << client->request.get_method() << std::endl; // Debug print of request method and content length

	open_pipe(cgi_state, cgi_state->fdo); // Always open the output pipe for CGI response
	if (is_post_with_body) {
		open_pipe(cgi_state, cgi_state->fdi);
	}

	if (DEBUG) std::cout << "  Pipes opened successfully. Forking process..." << std::endl; // Debug print before forking
	pid_t pid = fork();
	if (pid < 0) {
		close_pipes(cgi_state);
		throw std::runtime_error("fork error");
	}

	else if (pid == 0) { // Child process
		if (DEBUG) std::cout << GREEN << "In CGI child process. Setting up pipes and executing CGI script..." << RESET << std::endl; // Debug print in child process
		int null_fd = open("/dev/null", O_RDONLY);
		if (null_fd == -1) {
			close_pipes(cgi_state);
			throw std::runtime_error("Failed to open /dev/null");
		}
		size_t last_slash_pos = cgi_path.find_last_of('/');
		std::string cgi_dir = cgi_path.substr(0, last_slash_pos);
		std::string cgi_script = cgi_path.substr(last_slash_pos + 1);
		char **envp = build_envp(env_map);
		char **argv = build_args(cgi_script, bin_path);
		dup2(null_fd, STDERR_FILENO); // Redirect CGI child's stderr to /dev/null if there's no request body
		close(null_fd);
		dup2(cgi_state->fdo[1], STDOUT_FILENO); // Redirect CGI child's stdout to write end of response pipe
		if (is_post_with_body)
			dup2(cgi_state->fdi[0], STDIN_FILENO); // Redirect CGI child's stdin to read end of request pipe
		chdir(cgi_dir.c_str());
		execve(bin_path.c_str(), argv, envp);
		free_envp(envp);
		free_args(argv);
		// WARNING: more cleaning needed
		exit(1); // If exec fails
	}

	// TODO: Check if all fd are closed
	// parent process
	if (is_post_with_body) {
		if (DEBUG) std::cout << GREEN << "In CGI parent process. Closing unused pipe ends and setting up epoll..." << RESET << std::endl; // Debug print in parent process
		close(cgi_state->fdi[0]); // close read end of request pipe in parent
		set_non_blocking(cgi_state->fdi[1]);
	}
	close(cgi_state->fdo[1]); // close write end of response pipe in parent

	set_non_blocking(cgi_state->fdo[0]);

	// Add fdo[0] to epoll for monitoring CGI output
	struct epoll_event event;
	event.events = EPOLLIN;
	EpollData* data = new EpollData(cgi_state->fdo[0], CGI_PIPE, client);
	// data->client = client;
	// data->fd = cgi_state->fdo[0];
	// data->type = CGI_PIPE;
	event.data.ptr = data;
	data->client->cgi_state = cgi_state; // Store CGI state in EpollData for access in event handler

	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cgi_state->fdo[0], &event) == -1) {
		if (is_post_with_body && cgi_state->fdi[1] != -1) close(cgi_state->fdi[1]);
		close(cgi_state->fdo[0]);
		kill(pid, SIGKILL);
		delete data;
		throw std::runtime_error("epoll_ctl error");
	} else if (DEBUG) {
		std::cout << GREEN << "Successfully added CGI output pipe to epoll for monitoring." << RESET << std::endl; // Debug print after successfully adding to epoll
	}

	if (is_post_with_body) {
		// If there's a request body that hasn't been fully sent to the CGI child yet, we should also add the write end of the request pipe to epoll for monitoring when it's ready to send more data.
		set_non_blocking(cgi_state->fdi[1]);
		struct epoll_event req_event;
		req_event.events = EPOLLOUT;
		EpollData* req_data = new EpollData(client->cgi_state->fdi[1], CGI_PIPE, client);
		// req_data->client = client;
		// req_data->fd = cgi_state->fdi[1];
		// req_data->type = CGI_PIPE;
		req_data->client->cgi_state = cgi_state;
		req_event.data.ptr = req_data;

		if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cgi_state->fdi[1], &req_event) == -1) {
			if (cgi_state->fdi[1] != -1) close(cgi_state->fdi[1]);
			close(cgi_state->fdo[0]);
			kill(pid, SIGKILL);
			delete data;
			delete req_data;
			throw std::runtime_error("epoll_ctl error");
		} else if (DEBUG) {
			std::cout << GREEN << "Successfully added CGI request pipe to epoll for monitoring." << RESET << std::endl; // Debug print after successfully adding request pipe to epoll
		}
	}

	// Update cgi_state with the fork result and return
	cgi_state->pid = pid;
	cgi_state->req_w_fd = cgi_state->fdi[1];
	cgi_state->res_r_fd = cgi_state->fdo[0];
	if (DEBUG) std::cout << GREEN << "CGI execution started successfully with PID " << pid << "." << RESET << std::endl; // Debug print of CGI execution start
	return cgi_state;
}