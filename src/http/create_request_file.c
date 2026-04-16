#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main()
{
	int fd = open("test.txt", O_WRONLY | O_CREAT, 0644);
    char *s = "POST /update-profile HTTP/1.1\r\nHost: localhost:8080\r\nUser-Agent: Mozilla/5.0\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: 23\r\nCookie: sessionid=abc123xyz; theme=dark; last_visit=2026-04-03\r\nConnection: keep-alive\r\n\r\nname=Radouane&age=22";

    write(fd, s, strlen(s));
    close(fd);
	return 0;
}
