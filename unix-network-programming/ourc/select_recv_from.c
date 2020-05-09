#include "common.h"

int readable_timeo(int fd, int sec)
{
	fd_set rset;

	struct timeval tv;

	FD_ZERO(&rset);

	FD_SET(fd, &rset);

	tv.tv_sec = sec;
	tv.tv_usec = 0;

	return select(fd+1,&rset,NULL,NULL,&tv);
}

int main()
{
	int sockfd;
	struct sockaddr_in serveraddr, cliaddr;
	int select_result;
	int n;
	socklen_t clilen;
	char recvline[MAXLINE];
	char sendline[MAXLINE];
	//create udp socket
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(LOCAL);
	serveraddr.sin_port = htons(SERVPORT);

	bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));

	for (;;)
	{
		clilen = sizeof(cliaddr);
		if ((select_result = readable_timeo(sockfd, 10)) <= 0) {
			fprintf(stderr,"socket timeout\n");
		}
		else {
			n = recvfrom(sockfd, sendline, MAXLINE, 0, (struct sockaddr *)&cliaddr, &clilen);
			if (n < 0)
			{
				if (errno == EINTR) {
					continue;
				}
				else {
					printf("recv error\n");
				}
			}
		}


	}
}