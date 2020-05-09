#include "common.h"
#include <sys/types.h>
#include <arpa/inet.h>

//gethostbyname
/*
int main(int argc, char** argv)
{
	struct hostent* result;
	char **pptr;
	char **addr_list;
	result = gethostbyname("www.baidu.com");
	printf("%s\n", result->h_name);
	pptr = result->h_aliases;
	char str[INET_ADDRSTRLEN];
	while ((*pptr) != NULL)
	{
		printf("%s\n", *pptr);
		pptr++;
	}

	addr_list = result->h_addr_list;
	while (*addr_list != NULL)
	{
		inet_ntop(result->h_addrtype, (void*)*addr_list, str, sizeof(str));
		printf("%s\n", str);
		addr_list++;
	}
}*/

//gethostbyaddr
int main(int argc, char** argv)
{
	/*char               buf[100];
	int                ret = 0;
	struct addrinfo    hints;
	struct addrinfo    *res, *curr;
	struct sockaddr_in *sa;

	bzero(&hints, sizeof(hints));
	hints.ai_flags = AI_CANONNAME;
	hints.ai_family = AF_INET;
	if (gethostname(buf, sizeof(buf)) < 0) {
		perror("gethostname");
		return -1;
	}
	printf("%s\n", buf);
	if ((ret = getaddrinfo(buf, NULL, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
		return -1;
	}
	curr = res;
	while (curr && curr->ai_canonname) {
		sa = (struct sockaddr_in *)curr->ai_addr;
		printf("name: %s\nip:%s\n\n", curr->ai_canonname,
			inet_ntop(AF_INET, &sa->sin_addr.s_addr, buf, sizeof(buf)));
		curr = curr->ai_next;
	}*/

	char* domain = "www.baidu.com";
	struct servent* serv;
	char** serv_aliases;
	serv = getservbyname("http", "tcp");
	if (serv == NULL)
	{
		printf("getservbyname error\n");
		exit(-1);
	}
	serv_aliases = serv->s_aliases;
	printf("%s\n", serv->s_name);
	while (*serv_aliases != NULL)
	{
		printf("%s\n", *serv_aliases);
		serv_aliases++;
	}
	printf("%d\n", serv->s_port);
	printf("%s\n", serv->s_proto);
	return 0;
}