//imane 's file

#include "Router.hpp"
#include <sys/stat.h>//for stat function to check if file exists and if it's a directory
#include <dirent.h>//for opendir and readdir to list directory content

Router::Router(Config *config)
{
    this->config = config;
    this->server_block = nullptr;
}