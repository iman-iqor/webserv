//imane 's file

#include"Router.hpp"

Location* Router::findMatchingLocation(const std::string &path)
{
    if(!server_block)
    {
        return NULL;
    }
    Location* best_match = NULL;
    size_t longest_match = 0;

    for(size_t i = 0; i< server_block->locations.size();i++)
    {
        Location* loc = &server_block->locations[i];

        //check if path starts with loca]tion path
        if(path.find(loc->path) == 0)
        {
            if(loc->path.length() >longest_match )
            {
                longest_match = loc->path.length();
                best_match = loc;
            }
        }
    }

    return best_match;
}
bool Router::isMethodAllowed(const std::string &method,Location* location)
{
    if(!location)
        return false;
    
    if(location->methods.empty())//there is no methods dir libghiti
        return true;
    
    for(cons )
}