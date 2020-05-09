#include "web.h"

int main(int argc, char **argv)
{
	int i, fd, n, maxconn, flags, error;

	char buf[MAXLINE];

	fd_set rs, ws;

	if (argc < 5)
		sys_err("usage:web <#conn><hostname><homepage><file1>...");

	maxconn = atoi(argv[1]);

	nfiles = min(argc - 4, MAXFILES);

	//初始化文件结构体
	for (i = 0; i < nfiles; i++) {
		file[i].f_name = argv[i + 4];
		file[i].f_host = argv[2];
		file[i].f_flags = 0;
	}

	printf("nfiles = %d\n",nfiles);

	home_page(argv[2],argv[3]);
}