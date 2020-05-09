#include "common.h"

int main(int argc, char** argv)
{
	int sockfd;

	int len;
	sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);

	struct sockaddr_un serveraddr,cliaddr;

	serveraddr.sun_family = AF_LOCAL;

	strncpy(serveraddr.sun_path,argv[1],sizeof(serveraddr.sun_path)-1);

	unlink(argv[1]);

	bind(sockfd,(struct sockaddr*)&serveraddr,sizeof(serveraddr));

	len = sizeof(cliaddr);

	getsockname(sockfd,(struct sockaddr *)&cliaddr,&len);

	printf("biund name = %s,returned len = %d\n", cliaddr.sun_path, len);

	exit(0);
}