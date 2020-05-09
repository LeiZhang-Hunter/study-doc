#include "common.h"

int main()
{
	int sockfd;
	struct sockaddr_in serveraddr, cliaddr;
	socklen_t clilen;
	char recvline[MAXLINE];
	char sendline[MAXLINE];
	//create udp socket
	sockfd = socket(AF_INET,SOCK_DGRAM,0);
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(LOCAL);
	serveraddr.sin_port = hotns(SERVPORT);
	
	bind(sockfd,&serveraddr,sizeof(serveraddr));

	for (;;)
	{
		clilen = sizeof(cliaddr);
		n = recvfrom(sockfd, sendline,MAXLINE,0,&cliaddr,&clilen);
		if (n < 0)
		{
			if (errno == EINTR) {
				continue;
			}
			else {
				sys_err();
			}
		}
		

	}
}