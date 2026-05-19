#include"Server.hpp"

void  Server::handleCGI(EpollData* data,uint32_t events)
{
    (void) data;
    if(events & EPOLLERR)
    {
        // closeCGI(data);
        return;
    }
    if(events & EPOLLIN)
    {
        // readCGIOutput(data);
    }
    if(events & EPOLLHUP)
    {
        //cgi closed pipe
        //usually means CGI finished
        // finalizeCGIResponse(data);
    }
}