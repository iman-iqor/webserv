#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include "../http/Exceptions.hpp"

std::string trim(const std::string &s);
bool has_any(const std::string &s, const std::string &chars);
std::string& to_lower(std::string &s);
bool has_other(const std::string &s, std::string &chars);

#endif // UTILS_HPP