#include"Server.hpp"
#include <signal.h>
#include <sys/wait.h>
#include "../http/Exceptions.hpp"
#include "../http/CgiHandler.hpp"

#define BUFFER_SIZE 4096
typedef struct CgiResponse_s CgiResponse_t;

CgiResponse_t *parse_cgi_response(const std::string &cgi_output) {
	// parse CGI headers
	CgiResponse_t *response = new CgiResponse_t;
	size_t header_end = cgi_output.find("\r\n\r\n");
	if (header_end == std::string::npos) {
		throw BadGatewayException("Invalid CGI response: missing header-body separator");
	}

	std::string header_str = cgi_output.substr(0, header_end);
	response->body = cgi_output.substr(header_end + 4);

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
			delete response;
			throw BadGatewayException("Invalid CGI response: malformed header line");
		}
		std::string key = line.substr(0, colon_pos);
		std::string value = line.substr(colon_pos + 1);
		key.erase(key.find_last_not_of(" \t\r\n") + 1);
		value.erase(0, value.find_first_not_of(" \t\r\n"));
		std::string lkey = key;
		to_lower(lkey);
		if (lkey == "status") {
			if (value.find_first_not_of("0123456789") != std::string::npos) {
				delete response;
				throw BadGatewayException("Invalid CGI response: non-numeric status code");
			}
			response->status_code = std::atoi(value.c_str());
		}
		else {
			if (lkey != "content-type") {
				response->headers[key] = value;
			}
		}
	}
	return response;
}

void  Server::handleCGI(EpollData* data, uint32_t events)
{
	if (!data || !data->client)
		return;
	
	Client* client = data->client;
	CgiState_t *cgi_state = client->cgi_state;
	
	if (events & EPOLLERR)
	{
		// Handle error
		epoll_ctl(epoll_fd, EPOLL_CTL_DEL, data->fd, NULL);
		close(data->fd);
		close_pipes(cgi_state);
		kill(cgi_state->pid, SIGKILL);
		waitpid(cgi_state->pid, NULL, 0);
		delete data;
		delete cgi_state;
		client->cgi_state = NULL;
		return;
	}
	
	if (events & EPOLLIN)
	{
		// Read CGI output
		char buffer[BUFFER_SIZE];
		ssize_t bytes_read = read(cgi_state->res_r_fd, buffer, BUFFER_SIZE - 1);
		
		if (bytes_read < 0)
		{
			throw InternalServerErrorException("Failed to read from CGI output pipe");
		}
		if (bytes_read == 0)
		{
			// EOF, CGI finished writing
			if (cgi_state->res_r_fd != -1)
			{
				epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cgi_state->res_r_fd, NULL);
				close(cgi_state->res_r_fd);
				cgi_state->res_r_fd = -1;
			}
			cgi_state->ready_to_send = true;
			// client->response = parse_cgi_response(cgi_state->cgi_output);
			client->ready_to_send = true;
		}
		else
		{
			buffer[bytes_read] = '\0';
			cgi_state->cgi_output.append(buffer);
		}
	}
	
	if (events & EPOLLOUT)
	{
		// Write request body to CGI input
		if (cgi_state->req_w_fd != -1 && cgi_state->body_sent < client->request.get_content_length())
		{
			const std::string& body = client->request.get_body();
			ssize_t bytes_to_send = client->request.get_content_length() - cgi_state->body_sent;
			ssize_t bytes_written = write(cgi_state->req_w_fd, body.c_str() + cgi_state->body_sent, bytes_to_send);
			
			if (bytes_written > 0)
			{
				cgi_state->body_sent += bytes_written;
			}
			
			// Check if all body has been sent
			if (cgi_state->body_sent >= client->request.get_content_length())
			{
				epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cgi_state->req_w_fd, NULL);
				close(cgi_state->req_w_fd);
				cgi_state->req_w_fd = -1;
			}
		}
	}
	
	if (events & EPOLLHUP)
	{
		// CGI process closed the pipe, finalize response
		if (cgi_state->res_r_fd != -1)
		{
			epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cgi_state->res_r_fd, NULL);
			close(cgi_state->res_r_fd);
			cgi_state->res_r_fd = -1;
		}
		if (cgi_state->req_w_fd != -1)
		{
			epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cgi_state->req_w_fd, NULL);
			close(cgi_state->req_w_fd);
			cgi_state->req_w_fd = -1;
		}
		
		// Wait for CGI child process to finish
		int status;
		waitpid(cgi_state->pid, &status, 0);
		
		// Build response from CGI output
		cgi_state->ready_to_send = true;
		// client->response = parse_cgi_response(cgi_state->cgi_output);
		client->ready_to_send = true;
		
		// Clean up EpollData
		delete data;
		delete cgi_state;
		client->cgi_state = NULL;
	}
}