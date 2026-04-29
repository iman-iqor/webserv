
#include <iostream>
#include <string>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <sstream>
#include <cerrno>
#include <cstring>

#define BUFFER_SIZE 4096
#define SIZE 23

int randomInt() {
    static bool seeded = false;
    if (!seeded) {
        std::srand(static_cast<unsigned int>(std::time(NULL)));
        seeded = true;
    }
    return (std::rand() % 23);
}

static bool writeAll(int fd, const std::string &data) {
    size_t total_written = 0;
    while (total_written < data.size()) {
        ssize_t written = write(fd, data.c_str() + total_written, data.size() - total_written);
        if (written < 0) {
            return false;
        }
        total_written += static_cast<size_t>(written);
    }
    return true;
}

static bool readFileToString(const std::string &path, std::string &content) {
    std::ifstream file(path.c_str());
    if (!file.is_open()) {
        return false;
    }
    std::ostringstream stream;
    stream << file.rdbuf();
    content = stream.str();
    return true;
}

void doit(std::string &final, std::string &_buf) {
    size_t pos = 0;
    while (true) {
        size_t n = _buf.find("\r\n", pos);
        if (n == std::string::npos) {
            if (pos != 0) {
                _buf = _buf.substr(pos); // keep the unprocessed part of the buffer starting from the current position, which may contain a partial chunk size line or chunk data that has not been fully parsed yet. This allows the parser to continue processing the remaining data when more bytes arrive in subsequent reads.
                pos = 0; // reset the position to the beginning of the new buffer
            }
            break; // No more chunks to process
        }
        std::string hex = _buf.substr(pos, n - pos);
        if (hex.find_first_not_of("0123456789aAbBcCdDeEfF") != std::string::npos) {
            std::cout << "Invalid chunk size: [" << hex << "]" << std::endl;
            break; // Invalid hex value
        }
        pos = n + 2; // Move past the chunk size line
        int chunk_size = std::strtol(hex.c_str(), NULL, 16);
        if (chunk_size == 0) {
            std::cout << "Last chunk received" << std::endl;
            break; // Last chunk
        }

        if (_buf.size() - pos >= static_cast<size_t>(chunk_size + 2)) {
            std::cout << "Processing chunk of size: " << chunk_size << std::endl;
            final += _buf.substr(pos, chunk_size); // Append the chunk data to final
            pos += chunk_size + 2; // Move past the chunk data and the following "\r\n"
        } else {
            std::cout << "Incomplete chunk received, waiting for more data" << std::endl;
            final += _buf.substr(pos); // Append the remaining buffer to final
            chunk_size -= _buf.size() - pos; // Update the remaining chunk size
            pos = _buf.size(); // Move to the end of the buffer
            break; // Wait for more data to arrive
        }

    }
}

int main() {
    std::string final;
    std::string _buf;
    std::string str;
    if (!readFileToString("text.txt", str)) {
        std::cerr << "Failed to open text.txt: " << std::strerror(errno) << std::endl;
        return 1;
    }


    int fd[2];
    if (pipe(fd) == -1) {
        std::cerr << "Pipe failed!" << std::endl;
        return 1;
    }
    int pid = fork();
    if (pid == 0) {
        // Child process
        close(fd[0]);
        while (true) {
            int chunk_size = randomInt();
            if (chunk_size == 0) {
                continue; // Skip sending empty chunks
            }
            if (str.empty()) {
                break; // No more data to send
            }
            if (chunk_size > static_cast<int>(str.size())) {
                chunk_size = static_cast<int>(str.size());
            }
            std::string chunk_data = str.substr(0, chunk_size);
            str.erase(0, chunk_size); // Remove the sent chunk from the original string
            std::ostringstream hex_size;
            hex_size << std::hex << chunk_size;
            std::string chunk = hex_size.str() + "\r\n" + chunk_data + "\r\n";
            if (!writeAll(fd[1], chunk)) {
                std::cerr << "Child write failed" << std::endl;
                close(fd[1]);
                return 1;
            }
            std::cout << "Child sent chunk of size: " << chunk_size << std::endl;
            usleep(100000); // Sleep for a short time to simulate delay
        }
        if (!writeAll(fd[1], "0\r\n\r\n")) {
            std::cerr << "Child failed to send terminating chunk" << std::endl;
            close(fd[1]);
            return 1;
        }
        close(fd[1]);
        return 0;
    } else if (pid > 0) {
        // Parent process
        close(fd[1]);
        while (true) {
            char buffer[BUFFER_SIZE];
            ssize_t bytesRead = read(fd[0], buffer, BUFFER_SIZE - 1);
            if (bytesRead < 0) {
                std::cerr << "Read failed!" << std::endl;
                return 1;
            }
            if (bytesRead == 0) {
                std::cout << "No more data to read, exiting." << std::endl;
                break; // End of data
            }
            buffer[bytesRead] = '\0'; // Null-terminate the buffer
            std::cout << "Read " << bytesRead << " bytes: [" << buffer << "]" << std::endl;
            _buf += buffer;
            doit(final, _buf);
        }
        std::ofstream outFile("output.txt");
        if (!outFile.is_open()) {
            std::cerr << "Failed to open output.txt for writing: " << std::strerror(errno) << std::endl;
            return 1;
        }
        outFile << final;
        outFile.close();
        close(fd[0]);
        std::cout << "Decoded payload size: " << final.size() << std::endl;
    } else {
        std::cerr << "Fork failed!" << std::endl;
        return 1;
    }

    return 0;
}