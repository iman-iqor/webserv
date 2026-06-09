#ifndef UTILS_HPP
#define UTILS_HPP

#include <sstream>
#include <iostream>
#include <string>
#include <iomanip>
#include <cctype>
#include "../http/Exceptions.hpp"
#include "../config/Config.hpp"

std::string trim(const std::string &s);
bool has_any(const std::string &s, const std::string &chars);
std::string& to_lower(std::string &s);
std::string to_upper(const std::string &s);
bool has_other(const std::string &s, std::string &chars);
std::string ft_itoa(int n);
bool method_is_valid(const std::string& method);
void print_safe_string(const std::string& str);

Location *find_location(ServerBlock *server_block, const std::string& path);

#endif // UTILS_HPP