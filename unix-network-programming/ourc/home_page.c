#include "web.h"

void home_page(const char *host, const char *fname)
{
	int fd, n;
	char line[MAXLINE];

	fd = connect(host,SERV);

	n = snprintf(line,sizeof(line),GET_CMD,fname);

	write(fd, line, n);

	for (;;)
	{
		if ((n = read(fd, line, MAXLINE)) == 0)
		{
			break;
		}

		printf("read %d byters\n", n);
	}

	printf("end of file home_page");
	close(fd);
}