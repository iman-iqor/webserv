#include "utils.hpp"

bool method_is_valid(const std::string& method) {
	return (method == "GET" || method == "POST" || method == "DELETE");
}