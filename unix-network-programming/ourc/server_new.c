#include "common.h"

int main()
{
	int clifd;
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	socklen_t len;
	int connfd;
	int new_fd;

	int i;
	int maxi;
	struct sockaddr_in serversock, clisock;
	bzero(&serversock, sizeof(serversock));
	int nready;
	size_t n;
	char buf[MAXLINE];
	pid_t pid;
	serversock.sin_port = htons(SERVPORT);
	serversock.sin_family = AF_INET;
	serversock.sin_addr.s_addr = inet_addr(LOCAL);

	struct pollfd client[OPEN_MAX];

	bind(sockfd, (struct sockaddr *)&serversock, sizeof(serversock));

	listen(sockfd, 10);

	for (i = 1; i < OPEN_MAX; i++)
	{
		client[i].fd = -1;
	}

	client[0].fd = sockfd;
	client[0].events = POLLRDNORM;

	maxi = 0;

	for (;;)
	{
		printf("read roll\n");
		nready = poll(client,maxi+1, INFTIM);
		printf("read ok\n");
		if (client[0].revents & POLLRDNORM)
		{
			len = sizeof(clisock);
			connfd = accept(sockfd, (struct  sockaddr *)&clisock, &len);

			for (i = 1; i < OPEN_MAX; i++)
			{
				

				if (client[i].fd < 0)
				{
					printf("connect %d\n", connfd);
					client[i].fd = connfd;
					client[i].events = POLLRDNORM;
					break;
				}
			}

			if (i == OPEN_MAX)
			{
				printf("%d\n", i);
				printf("open too much\n");
				exit(-1);
			}

			if (i > maxi)
				maxi = i;

			printf("maxi:%d\n",maxi);

			if (--nready <= 0)
				continue;
		}

		for (i = 1; i <= maxi; i++)
		{
			printf("maxi client:%d\n", maxi);
			if ((new_fd = client[i].fd) < 0)
				continue;

			if (client[i].revents & (POLLRDNORM | POLLERR))
			{
				printf("read %d\n", new_fd);
				if ((n = read(new_fd, buf, MAXLINE)) < 0)
				{
					if (errno = ECONNRESET) {
						close(new_fd);
						client[i].fd = -1;
					}
					else {
						printf("read error\n");
					}
				}
				else if (n == 0) {
					close(new_fd);
					client[i].fd = -1;
				}
				else {
					printf("writen begin\n");
					writen(new_fd, buf, n);
				}

				if (--nready <= 0)
				{
					break;
				}
			}
		}
	}
}