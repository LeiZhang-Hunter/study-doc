#include "common.h"
#define BUFSIZE 255
int my_open(const char* pathname, int mode);

ssize_t readfd(int fd, void* ptr, size_t nbytes, int *recvfd);

int main(int argc, char** argv)
{
	int fd, n;
	char buf[BUFSIZE];
	printf("%s\n", argv[1]);
	if (argc != 2)
	{
		sys_err("usage mycat <pathname>");
	}

	if ((fd = my_open(argv[1], O_RDONLY)) < 0)
	{
		printf("cannot 1111 %s\n", argv[1]);;
		printf("cannot open %s\n", argv[1]);;
		exit(-1);
	}

	while ((n = read(fd, buf, BUFSIZE)) > 0)
		write(STDOUT_FILENO,buf,n);

	exit(0);
}

ssize_t readfd(int fd,void* ptr,size_t nbytes,int *recvfd)
{
	struct msghdr msg;
	struct iovec iov[1];
	int n;

	msg.msg_name = NULL;
	msg.msg_namelen = 0;

	iov[0].iov_base = ptr;
	iov[0].iov_len = nbytes;

	msg.msg_iov = iov;
	msg.msg_iovlen = 1;

	struct cmsghdr* imptr;
	union {
		struct cmsghdr cm;
		char control[CMSG_SPACE(sizeof(int))];
	}control_un;

	msg.msg_control = control_un.control;
	msg.msg_controllen = sizeof(control_un.control);
	
	
	if ((n = recvmsg(fd, &msg, 0)) <= 0)
	{
		printf("recvmsg main fd:%d\n", fd);
		return n;
	}

	imptr = CMSG_FIRSTHDR(&msg);

	if ((imptr != NULL) && (imptr->cmsg_len == CMSG_LEN(sizeof(int))))
	{
		if (imptr->cmsg_level != SOL_SOCKET)
		{
			printf("control level != SOL_SOCKET");
			exit(-1);
		}

		if (imptr->cmsg_type != SCM_RIGHTS)
		{
			printf("control type != SCM_RIGHTRS");
			exit(-1);
		}
		
		*recvfd = *((int *)CMSG_DATA(imptr));
	}
	else {
		*recvfd = -1;
	}
	return n;
}

int my_open(const char* pathname,int mode)
{
	int fd, sockfd[2], status;
	pid_t childpid;
	char c, argsockfd[10], argmode[10];

	socketpair(AF_LOCAL,SOCK_STREAM,0,sockfd);

	if ((childpid = fork()) == 0)
	{
		close(sockfd[0]);
		//将数字全部格式化为字符串
		snprintf(argsockfd,sizeof(argsockfd),"%d",sockfd[1]);
		snprintf(argmode, sizeof(argmode), "%d", mode);

		printf("argsockfd n:%s\n", argsockfd);
		printf("argmode n:%s\n", argmode);
		printf("argmode n:%s\n", pathname);
		execl("./openfile", "openfile", argsockfd, pathname, argmode, (char*)NULL);
		sys_err("execl error");
	}

	close(sockfd[1]);

	waitpid(childpid,&status,0);
	printf("status n:%d\n", status);


	if (WIFEXITED(status) == 0)
		sys_err("child did not terminate");

	if ((status = WEXITSTATUS(status)) == 0)
	{
		readfd(sockfd[0],&c,1,&fd);
	}
	else {
		errno = status;
		fd = -1;
	}

	close(sockfd[0]);
	return fd;
}