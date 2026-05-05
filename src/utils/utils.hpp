#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include "../http/Exceptions.hpp"
#include "../config/Config.hpp"

std::string trim(const std::string &s);
bool has_any(const std::string &s, const std::string &chars);
std::string& to_lower(std::string &s);
bool has_other(const std::string &s, std::string &chars);

bool method_is_valid(const std::string& method);

Location *find_location(ServerBlock *server_block, const std::string& path);

#endif // UTILS_HPP