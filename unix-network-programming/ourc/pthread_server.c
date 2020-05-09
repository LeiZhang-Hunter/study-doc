#include "common.h"

static void* doit(void*);

static pthread_key_t r1_key;
static pthread_once_t r1_once = PTHREAD_ONCE_INIT;

static void readline_destructor(void* ptr)
{
	free(ptr);
}

static void readline_once(void)
{
	pthread_key_create(&r1_key,readline_destructor);
}

typedef struct {
	int r1_cnt;
	char *r1_bufptr;
	char r1_buf[MAXLINE];
}Rline;

static size_t my_read(Rline *tsd, int fd, char* ptr)
{
	if (rsd->r1_cnt <= 0)
	{
	again:
		if ((tsd->r1_cnt = read(fd, tsd->r1_buf, MAXLINE) < 0))
		{
			if (errno == EINTR)
			{
				goto again;
			}
			else {
				return -1;
			}
		}
		else if (tsd->r1_cnt == 0)
		{
			return 0;
		}

		tsd->r1_bufptr = tsd->r1_buf;
	}

	tsd->r1_cnt--;
	*ptr = *tsd->r1_bufptr++;
	return 1;
}

ssize_t readline(int fd, void* vptr, size_t maxlen)
{
	size_t n, rc;
	char c, *ptr;
	Rline *tsd;

	pthread_once(&r1_once,readline_once);

	if ((tsd = pthread_getspecific(r1_key)) == NULL)
	{
		tsd = calloc(1, sizeof(Rline));
		pthread_setspecific(r1_key, tsd);
	}
}

int main()
{
	int sockfd;
	int addrlen, len;
	int connfd;
	int *iptr;
	pthread_t tid;
	sockfd = socket(AF_INET,SOCK_STREAM,0);
	if (sockfd <= 0)
	{
		sys_err("create socket error");
	}

	struct sockaddr_in servaddr,*cliaddr;
	addrlen = sizeof(servaddr);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_port = htons(8500);
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr("0.0.0.0");

	if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
	{
		sys_err("bind socket error");
	}

	if (listen(sockfd, 128) == -1)
	{
		sys_err("listen socket error");
	}

	
	cliaddr = malloc(sizeof(cliaddr));


	for (;;)
	{
		len = addrlen;
		iptr = malloc(sizeof(int));
		*iptr = accept(sockfd,(struct sockaddr *)&cliaddr,&len);
		pthread_create(&tid,NULL,&doit,iptr);
		
	}

	return 0;
}

static void* doit(void* arg)
{
	int connfd;

	connfd = *((int*)arg);
	free(arg);

	pthread_detach(pthread_self());
	close(connfd);
	return NULL;
}