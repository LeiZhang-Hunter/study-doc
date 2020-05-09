#iovec

####简介

I/O vector，与readv和wirtev操作相关的结构体。readv和writev函数用于在一次函数调用中读、写多个非连续缓冲区。有时也将这两个函数称为散布读（scatter read）和聚集写（gather write）。

	#include <sys/uio.h>
	/* Structure for scatter/gather I/O. */
	struct iovec{
	     void *iov_base; /* Pointer to data. */
	     size_t iov_len; /* Length of data. */
	};

	ssize_t readv(int fd, const struct iovec *iov, int iovcnt);
	ssize_t writev(int fd, const struct iovec *iov, int iovcnt);

	/*
	* 将三个独立的字符串一次写入终端。*
	* */
	#include <sys/uio.h>
	int main(int argc,char **argv)
	{
	    char part1[] = "This is iov";
	    char part2[] = " and ";
	    char part3[] = " writev test";
	    struct iovec iov[3];
	    iov[0].iov_base = part1;
	    iov[0].iov_len = strlen(part1);
	    iov[1].iov_base = part2;
	    iov[1].iov_len = strlen(part2);
	    iov[2].iov_base = part3;
	    iov[2].iov_len = strlen(part3);
	    writev(1,iov,3);
	    return 0;
	}