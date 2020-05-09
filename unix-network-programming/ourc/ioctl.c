#include "common.h"
#include <unistd.h>
#include <sys/ioctl.h>
int main()
{
	int flags = 0;
	int fd = socket(AF_INET,SOCK_STREAM,0);
	//ioctl(fd, FIONBIO, &flags);
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(4000);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	int result = connect(fd,(struct sockaddr*)&addr,sizeof(addr));
	printf("%d\n", result);
	char buf[MAXLINE];
	read(fd,buf,MAXLINE);
}