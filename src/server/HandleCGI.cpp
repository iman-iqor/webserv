#include"Server.hpp"
#include <signal.h>
#include <sys/wait.h>
#include "../http/Exceptions.hpp"
#include "../http/CgiHandler.hpp"
#include "../../webserv.h"

#define BUFFER_SIZE 4096
typedef struct CgiResponse_s CgiResponse_t;

CgiResponse_t parse_cgi_response(const std::string &cgi_output) {
	// parse CGI headers

    CgiResponse_t response;
    if (cgi_output.empty()) {
        response.body = "";
        response.status_code = 200;
        response.status_message = "OK";
        std::cout << YELLOW << "Warning: No headers found in CGI response. Treating entire output as body." << RESET << std::endl;
        return response;
    }
	if (DEBUG) std::cout << GREEN << "Parsing CGI response..." << RESET << std::endl; // Debug print of CGI response parsing
	size_t header_end = cgi_output.find("\r\n\r\n");
	if (header_end == std::string::npos) {
		header_end = 0;
	}
	if (DEBUG) std::cout << "  Found header-body separator at index " << header_end << "." << RESET << std::endl; // Debug print of header-body separator index

    std::string header_str;
    if (header_end != 0)
    { 
        header_str = cgi_output.substr(0, header_end + 4);
	response.body = cgi_output.substr(header_end + 4);
    }
    else
    {
        header_str = "";
        response.body = cgi_output; // Debug print of missing headers warning
    }
	if (DEBUG) std::cout << "  Extracted headers:\n" << header_str << RESET << std::endl; // Debug print of extracted headers

	size_t pos = 0;
	while (pos < header_str.length()) {
		size_t line_end = header_str.find("\r\n", pos);
		if (line_end == std::string::npos)
			break;
		
		std::string line = header_str.substr(pos, line_end - pos);
		pos = line_end + 2; // Move past the \r\n
		if (line.empty()) {
			continue; // Skip empty lines
		}
		size_t colon_pos = line.find(':');
		if (colon_pos == std::string::npos) {
			throw BadGatewayException("Invalid CGI response: malformed header line");
		}
		std::string key = line.substr(0, colon_pos);
		std::string value = line.substr(colon_pos + 1);
		key.erase(key.find_last_not_of(" \t\r\n") + 1);
		value.erase(0, value.find_first_not_of(" \t\r\n"));
		std::string lkey = key;
		to_lower(lkey);
		if (lkey == "status") {
			// Parse status line: e.g., "200 OK" or just "200"
			size_t space_pos = value.find(' ');
			if (space_pos != std::string::npos) {
				std::string code_str = value.substr(0, space_pos);
				if (code_str.find_first_not_of("0123456789") != std::string::npos) {
					throw BadGatewayException("Invalid CGI response: non-numeric status code");
				}
				response.status_code = std::atoi(code_str.c_str());
				response.status_message = value.substr(space_pos + 1);
			} else {
				response.status_code = std::atoi(value.c_str());
				// Keep default status message
			}
		}
		else {
			response.headers[key] = value;
		}
	}
	if (DEBUG) {
		std::cout << GREEN << "Parsed CGI response:\n";
		std::cout << "  Status code: " << response.status_code << "\n";
		std::cout << "  Status message: " << response.status_message << "\n";
		std::cout << "  Headers:\n";
		for (std::map<std::string, std::string>::iterator it = response.headers.begin(); it != response.headers.end(); ++it) {
			std::cout << "    " << it->first << ": " << it->second << "\n";
		}
		std::cout << "  Body length: " << response.body.length() << "\n" << RESET; // Debug print of body length
	}
	return response;
}

void  Server::handleCGI(EpollData* data, uint32_t events)
{
	if (DEBUG) std::cout << GREEN << "Handling CGI event for fd " << data->fd << " with events: " << events << RESET << std::endl; // Debug print of CGI event handling
	if (!data || !data->client)
		return;
	
	Client* client = data->client;
	CgiState_t *cgi_state = client->cgi_state;
	
	if (events & EPOLLERR)
	{
		// Handle error
		if (DEBUG) std::cerr << RED << "Error event on CGI pipe for client fd " << client->fd << ". Cleaning up CGI state." << RESET << std::endl; // Debug print of error event handling
		epoll_ctl(epoll_fd, EPOLL_CTL_DEL, data->fd, NULL);
		close(data->fd);
		close_pipes(cgi_state);
		kill(cgi_state->pid, SIGKILL);
		waitpid(cgi_state->pid, NULL, WNOHANG);
		delete data;
		delete cgi_state;
		client->cgi_state = NULL;
		return;
	}
	
	if (events & EPOLLIN)
	{
		if (DEBUG) std::cout << GREEN << "Readable event on CGI output pipe for client fd " << client->fd << ". Reading CGI output..." << RESET << std::endl; // Debug print of readable event handling
		// Read CGI output
		char buffer[BUFFER_SIZE];
		if (DEBUG && cgi_state == NULL) std::cerr << RED << "Warning: CGI state is NULL for client fd " << client->fd << " during EPOLLIN handling." << RESET << std::endl; // Debug print of null CGI state warning
		ssize_t bytes_read = read(cgi_state->res_r_fd, buffer, BUFFER_SIZE - 1);
		
		if (bytes_read < 0)
		{
			throw InternalServerErrorException("Failed to read from CGI output pipe");
		}
		if (bytes_read == 0)
		{
			if (DEBUG) std::cout << GREEN << "End of CGI output reached for client fd " << client->fd << "." << RESET << std::endl; // Debug print of end of CGI output
			// EOF, CGI finished writing
			if (cgi_state->res_r_fd != -1)
			{
				epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cgi_state->res_r_fd, NULL);
				close(cgi_state->res_r_fd);
				cgi_state->res_r_fd = -1;
			}
			cgi_state->ready_to_send = true;
			// CgiResponse_t cgi_response = parse_cgi_response(cgi_state->cgi_output);
			// Response res(*this);
			// res.handleCGIres(cgi_response);
			// client->response = res.build();
			// client->ready_to_send = true;

			// struct epoll_event event;
			// EpollData *data = new EpollData(client->fd, CLIENT, client);
			// event.data.ptr = data;
			// event.events = EPOLLOUT;
			// epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client->fd, &event);
			// delete cgi_state;
			// client->cgi_state = NULL;
		}
		else
		{
			if (DEBUG) std::cout << GREEN << "Read " << bytes_read << " bytes from CGI output for client fd " << client->fd << "." << RESET << std::endl; // Debug print of bytes read from CGI output
			buffer[bytes_read] = '\0';
			cgi_state->cgi_output.append(buffer);
		}
	}
	
	// if (events & EPOLLOUT)
	// {
	// 	if (DEBUG) std::cout << GREEN << "Writable event on CGI request pipe for client fd " << client->fd << ". Writing request body..." << RESET << std::endl; // Debug print of writable event handling
	// 	// Write request body to CGI input
	// 	if (cgi_state->req_w_fd != -1 && cgi_state->body_sent < client->request.get_content_length())
	// 	{
	// 		const std::string& body = client->request.get_body();
	// 		ssize_t bytes_to_send = client->request.get_content_length() - cgi_state->body_sent;
	// 		ssize_t bytes_written = write(cgi_state->req_w_fd, body.c_str() + cgi_state->body_sent, bytes_to_send);
	// 		if (bytes_written < 0) {
	// 			throw InternalServerErrorException("Failed to write to CGI input pipe");
	// 		}
	// 		if (bytes_written > 0)
	// 		{
	// 			cgi_state->body_sent += bytes_written;
	// 		}
			
	// 		// Check if all body has been sent
	// 		if (cgi_state->body_sent >= client->request.get_content_length())
	// 		{
	// 			epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cgi_state->req_w_fd, NULL);
	// 			close(cgi_state->req_w_fd);
	// 			cgi_state->req_w_fd = -1;
	// 		}
	// 	}
	// }
	
	if (events & EPOLLHUP)
	{
		if (DEBUG) std::cout << GREEN << "Hang-up event on CGI pipe for client fd " << client->fd << ". Finalizing CGI response and cleaning up." << RESET << std::endl; // Debug print of hang-up event handling
		// CGI process closed the pipe, finalize response
		if (cgi_state->res_r_fd != -1)
		{
			epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cgi_state->res_r_fd, NULL);
			close(cgi_state->res_r_fd);
			cgi_state->res_r_fd = -1;
		}
		
		// Wait for CGI child process to finish (non-blocking)
		int status;
		pid_t result = waitpid(cgi_state->pid, &status, WNOHANG);

		if (result == 0) {
			kill(cgi_state->pid, SIGKILL);
			waitpid(cgi_state->pid, &status, 0);
			std::cerr << YELLOW << "Warning: CGI child process was still running and has been killed." << RESET << std::endl; // Debug print of CGI child still running warning
		} else if (result == -1) {
			// Error (e.g. child already reaped, or invalid pid)
			perror("waitpid");
			throw InternalServerErrorException("Failed to wait for CGI child process");
		}

		if (DEBUG) {
			// TODO: verify if the behavior of the cgi response is the same when the child process is killed vs when it exits normally. If the child is killed, we might not get a complete response, so we should handle that case appropriately in the response handling code.
			if (WIFEXITED(status)) {
				std::cout << GREEN << "CGI child process exited with status " << WEXITSTATUS(status) << "." << RESET << std::endl; // Debug print of CGI child exit status
			} else if (WIFSIGNALED(status)) {
				std::cout << GREEN << "CGI child process killed by signal " << WTERMSIG(status) << "." << RESET << std::endl; // Debug print of CGI child killed by signal
			} else {
				std::cout << GREEN << "CGI child process still running or terminated abnormally." << RESET << std::endl; // Debug print of CGI child still running or abnormal termination
			}
		}
		
		// Build response from CGI output
		cgi_state->ready_to_send = true;
		CgiResponse_t cgi_response = parse_cgi_response(cgi_state->cgi_output);
		Response res(*this);
		res.handleCGIres(cgi_response);
		client->response = res.build();
		client->ready_to_send = true;

		struct epoll_event event;
		event.data.ptr = epoll_data[client->fd];
		event.events = EPOLLOUT;
		epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client->fd, &event);
		delete cgi_state;
		client->cgi_state = NULL;
	}
}