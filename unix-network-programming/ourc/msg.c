#include "common.h"
#define    HAVE_MSGHDR_MSG_CONTROL 

ssize_t writefd(int fd, void* ptr, size_t nbytes, int sendfd);

int main(int argc,char** argv)
{
	int fd;

	if (argc != 4)
	{
		printf("openfile <sockfd> <filename> <mode>");
		exit(-1);
	}

	if ((fd = open(argv[2], atoi(argv[3]))) < 0)
	{
		exit((errno > 0) ? errno :errno);
	}

	if (writefd(atoi(argv[1]), "", 1, fd) < 0)
	{
		printf("errno:%d\n",errno);

		exit((errno > 0) ? errno : 255);
	}

	printf("55555555\n");

	exit(0);
}

ssize_t writefd(int fd, void* ptr, size_t nbytes, int sendfd)
{
	struct msghdr msg;
	struct iovec iov[1];

	iov[0].iov_base = ptr;
	iov[0].iov_len = nbytes;

	msg.msg_name = NULL;
	msg.msg_namelen = 0;

#ifdef HAVE_MSGHDR_MSG_CONTROL
	union {
		struct cmsghdr cm;
		char control[CMSG_SPACE(sizeof(int))];
	}control_un;

	struct cmsghdr *cmptr;

	msg.msg_control = control_un.control;
	msg.msg_controllen = sizeof(control_un.control);

	//CMSG_FIRSTHDR的实现有许多种，返回cmsg_control的地址)
	cmptr = CMSG_FIRSTHDR(&msg);
	cmptr->cmsg_len = CMSG_LEN(sizeof(int));
	cmptr->cmsg_level = SOL_SOCKET;
	cmptr->cmsg_type = SCM_RIGHTS;
	*((int *)CMSG_DATA(cmptr)) = sendfd;
#else
	msg.msg_accrights = (caddr_t)&sendfd;
	msg.msg_accrightslen = sizeof(int);
#endif

	 int result = sendmsg(fd,&msg,0);
	 printf("result:%d\n", result);
	 return result;
}