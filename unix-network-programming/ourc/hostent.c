#include "common.h"

int main(int argc, char** argv)
{
	char *ptr, **pptr;
	char str[INET_ADDRSTRLEN];
	struct hostent *hptr;

	while (--argc > 0)
	{
		ptr = *++argv;

		if ((hptr = gethostbyname(ptr)) == NULL)
		{
			printf("gethostbyname error for host:%s,%s", ptr, hstrerror(errno));
			continue;
		}

		printf("official hostname:%s\n", hptr->h_name);

		for (pptr = hptr->h_aliases; *pptr != NULL; pptr++)
		{
			printf("\talias:%s\n", *pptr);
		}

		switch (hptr->h_addrtype)
		{
		case AF_INET:
			
			for (pptr = hptr->h_addr_list; *pptr != NULL; pptr++) {
				inet_ntop(hptr->h_addrtype, (void*)*pptr, str, sizeof(str));
				printf("%s\n", str);
			}

				
				//);
			break;
		default:
			sys_err("unknow address type\n");
			break;
		}
	}
	exit(0);
}