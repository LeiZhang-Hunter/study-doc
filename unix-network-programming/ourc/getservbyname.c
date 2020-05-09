#include "common.h"

int main(int argc, char** argv)
{
	int sockfd, n;
	char recvline[MAXLINE + 1];

	struct sockaddr_in servaddr;

	struct in_addr **pptr;

	struct in_addr *inetaddrp[2];

	struct in_addr inetaddr;

	struct hostent *hp;

	struct servent *sp;

	char str[INET_ADDRSTRLEN];

	if (argc != 3) {
		sys_err("usage:daytimetcpcli <hostname> <service>");
	}

	if ((hp = gethostbyname(argv[1])) == NULL)
	{
		if (inet_aton(argv[1], &inetaddr) == 0) {
			printf("hostname error for %s:%s", argv[1], hstrerror(h_errno));
			exit(-1);
		}
		else {
			inetaddrp[0] = &inetaddr;
			inetaddrp[1] = NULL;
			pptr = inetaddrp;
		}
	}
	else {
		pptr = (struct in_addr **)hp->h_addr_list;
	}

	if ((sp = getservbyname(argv[2], "tcp")) == NULL) {
		printf("getservbyname error for %s", argv[2]);
		exit(-1);
	}

	for (; *pptr != NULL; pptr++)
	{
		inet_ntop(hp->h_addrtype, (void*)*pptr, str, sizeof(str));
		printf("%s\n", str);
		printf("%d\n", sp->s_port);
		sockfd = socket(AF_INET, SOCK_STREAM, 0);

		bzero(&servaddr, sizeof(servaddr));

		servaddr.sin_family = AF_INET;

		servaddr.sin_port = (sp->s_port);

		memcpy(&servaddr.sin_addr, *pptr, sizeof(struct in_addr));

		//printf("trying %s\n", sock_ntop((struct sockaddr *)&servaddr, sizeof(servaddr)));

		if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == 0)
			break;

		printf("connect error\n");
		close(sockfd);
	}

	if (*pptr == NULL)
	{
		sys_err("unable to connect\n");
	}

	exit(0);
}