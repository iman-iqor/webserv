#include "utils.hpp"

Location *find_location(ServerBlock *server_block, const std::string& path)
{
	Location *best_match = NULL;
	size_t best_length = 0;

	for (size_t i = 0; i < server_block->locations.size(); i++) {
		const std::string& loc_path = server_block->locations[i].path;
		if (path.find(loc_path) == 0 && loc_path.length() > best_length) {
			best_match = &server_block->locations[i];
			best_length = loc_path.length();
		}
	}
	return best_match;
}
