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

void CgiHandler::close_pipes(int fdi[2], int fdo[2]) {
	if (fdi[0] != -1) close(fdi[0]);
	if (fdi[1] != -1) close(fdi[1]);
	if (fdo[0] != -1) close(fdo[0]);
	if (fdo[1] != -1) close(fdo[1]);
}

void CgiHandler::open_pipes(int fdi[2], int fdo[2])
{
	if (pipe(fdi) == -1 || pipe(fdo) == -1) {
		CgiHandler::close_pipes(fdi, fdo);
		throw std::runtime_error("pipe error");
	}
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

CgiState_t CgiHandler::start(
	Client *client,
	const std::string &cgi_path,
	std::string &bin_path,
	int epoll_fd,
	const std::map< std::string, std::string > &env_map
) {
	int req_pipe[2] = {-1, -1};
	int res_pipe[2] = {-1, -1};

	open_pipes(req_pipe, res_pipe);

	pid_t pid = fork();
	if (pid < 0) {
		close_pipes(req_pipe, res_pipe);
		throw std::runtime_error("fork error");
	}

	else if (pid == 0) { // Child process
		close(req_pipe[1]);	// close write end of request pipe in child 
		close(res_pipe[0]);	// close read end of response pipe in child

		dup2(req_pipe[0], STDIN_FILENO);
		dup2(res_pipe[1], STDOUT_FILENO);

		char **envp = build_envp(env_map);
		char **argv = build_args(cgi_path, bin_path);
		execve(bin_path.c_str(), argv, envp);
		free_envp(envp);
		free_args(argv);
		exit(1); // If exec fails
	}

	// TODO: Check if all fd are closed
	// parent process
	close(req_pipe[0]);

	set_non_blocking(res_pipe[0]);

	// Add res_pipe[0] to epoll for monitoring CGI output
	struct epoll_event event;
	event.events = EPOLLIN;
	EpollData* data = new EpollData;
	data->fd = res_pipe[0];
	data->type = CGI_PIPE;
	data->client = client;
	event.data.ptr = data;

	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, res_pipe[0], &event) == -1) {
		close(req_pipe[1]), close(res_pipe[0]);
		kill(pid, SIGKILL);
		delete data;
		throw std::runtime_error("epoll_ctl error");
	}

	if (client->request.get_method() == "POST" && client->request.get_content_length() > 0) {
		// If there's a request body that hasn't been fully sent to the CGI child yet, we should also add the write end of the request pipe to epoll for monitoring when it's ready to send more data.
		set_non_blocking(req_pipe[1]);
		struct epoll_event req_event;
		req_event.events = EPOLLOUT;
		EpollData* req_data = new EpollData;
		req_data->fd = req_pipe[1];
		req_data->type = CGI_PIPE;
		req_data->client = client;
		req_event.data.ptr = req_data;

		if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, req_pipe[1], &req_event) == -1) {
			close(req_pipe[1]), close(res_pipe[0]);
			kill(pid, SIGKILL);
			delete data;
			delete req_data;
			throw std::runtime_error("epoll_ctl error");
		}
	}
	else
		close(req_pipe[1]); // No body to send, close write end of request pipe in parent

	CgiState_t cgi_state = {
		.pid = pid,
		.req_w_fd = req_pipe[1],
		.res_r_fd = res_pipe[0],
		.cgi_output = "",
		.body_sent = 0,
		.ready_to_send = false
	};
	return cgi_state;
}