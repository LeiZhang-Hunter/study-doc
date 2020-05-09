#include "common.h"
#include <sys/types.h>
#include <arpa/inet.h>

int main(void)
{
	char               buf[100];
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
	}
}