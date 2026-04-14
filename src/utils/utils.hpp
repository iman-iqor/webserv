#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>

std::string trim(const std::string &s);
bool has_any(const std::string &s, const std::string &chars);
std::string& to_lower(std::string &s);

#endif // UTILS_HPP