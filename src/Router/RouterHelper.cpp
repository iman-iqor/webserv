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
            std::cout<<"found a match with location path: "<<loc->path<<std::endl;
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
    
    for(size_t i =0;i < location->methods.size();i++)
    {
        if(location->methods[i] == method)
            return true;
    }
    return false;
}
std::string Router::resolveFilePath(const std::string &path,Location *location)
{
    if(!location) return "";

    std::string relative_path = path;
    if(path.find(location->path) == 0)
    {
        std::cout<<"dkhal"<<std::endl;
        relative_path = path.substr(location->path.length());
        if(relative_path.empty())
        {
            std::cout<<"empty relative path"<<std::endl;
            relative_path = "/";
        }
    }
    std::cout<<"relative path: "<<relative_path<<std::endl;
    std::string full_path = location->root + "/" + relative_path;

    return full_path;
}

bool Router::fileExists(const std::string &path)
{
    return (access(path.c_str(),F_OK) == 0);
}

bool Router::isDirectory(const std::string &path)
{
    struct stat path_stat;

    if (stat(path.c_str(), &path_stat) != 0)
        return false;

    return S_ISDIR(path_stat.st_mode);
}
std::string Router::getFileExtension(const std::string &path)
{
    size_t dot = path.rfind('.');
    if(dot == std::string::npos)
        return"";
    return path.substr(dot);
}

bool Router::isExecutable(const std::string &path)
{
    return (access(path.c_str(),X_OK) == 0);
}

bool Router::isUploadAllowed(Location* location)
{
    if(!location) return false;
    return !location->upload_path.empty();
}

std::string Router::sizeToString(size_t value)
{
    std::stringstream ss;
    ss<<value;
    return ss.str();
}

std::string Router::timeToString(time_t value)
{
    std::stringstream ss;
    ss << value;
    return ss.str();
}
