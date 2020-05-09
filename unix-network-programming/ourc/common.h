#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/un.h>
#include <pthread.h>

#include <netdb.h>
#define SERVPORT 4000
#define LOCAL "127.0.0.1"
#define MAXLINE 255
#define OPEN_MAX 10
#define INFTIM -1

#define max( a, b) \
( (a) > (b)?(a) : (b) )

int readline(int fileFd, void* vptr, size_t maxlen)
{
	ssize_t n, rc;

	char c, *ptr;

	ssize_t result;

	ptr = vptr;
	for(n = 1;n<maxlen;n++)
	{

	again:
		result = read(fileFd,&c,1);
		if(result > 0)
		{
			*ptr++ = c;
			if(c == '\n'){
				break;
			}
		}

		if(result < 0)
		{
			if(errno == EINTR)
						goto again;

			return -1;
		}else if(result == 0)
		{
			*ptr = 0;
			return n-1;
		}


	}
	*ptr = 0;
	return n;
}

int writen(int fd, void* vptr, size_t n)
{
	size_t nleft;
	ssize_t nwrite;
	char* ptr = vptr;
	nleft = n;

	if (nleft > 0)
	{
		if ((nwrite = write(fd, ptr, nleft)) < 0)
		{
			if (errno = EINTR)
			{
				nwrite = 0;
			}
			else {
				return -1;
			}
		}

		nleft -= nwrite;
		ptr += nwrite;
	}
	return n;
}

int readn(int fd, void *vptr, size_t n)
{
	size_t nleft;
	ssize_t nread;

	char* ptr;

	ptr = vptr;
	nleft = n;

	while (nleft > 0)
	{
		if ((nread = read(fd, ptr, nleft)) <= 0)
		{
			if (errno = EINTR)
			{
				nread = 0;
			}
			else if (nread == 0)
			{
				break;
			}
			else {
				return -1;
			}
		}

		nleft = nleft - nread;
		ptr = ptr + nleft;

	}
	return nleft;
}

void sys_err(const char* str) {
	printf("%s\n", str);
	exit(-1);
}
