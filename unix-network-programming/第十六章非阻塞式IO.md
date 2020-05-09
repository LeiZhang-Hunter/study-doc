#第十六章非阻塞式IO

####16.1概述

套接字的默认状态式阻塞的。这意味着当发生一个不能立即完成的套接字调用的时候，其进程将会投入睡眠，等待相应操作完成。可能阻塞的套接字分为以下四类。

(1)输入的操作，包括read、readv、recv、recvfrom和recvmsg这5个函数，如果某进程对一个阻塞的TCP套接字调用这些函数中的一个，而且这个套接字缓冲区中没有数据可读，那么进程将会被投入睡眠，直到有一些数据到达。既然TCP是字节流协议，该进程的唤醒就是只要有字节到达，这些字节即可能是单个字节，也可能是一个完整的TCP分节中的数据。如果想等到某个固定数目的数据可读为止，既可以调用我们的readn也可以设置MSG_WAITALL标志位

既然UDP是数据报协议，如果一个阻塞的UDP套接字的接收缓冲区为空，对它调用输入函数的进程将被投入睡眠，直到有UDP数据报到达。

对于非阻塞的套接字，如果输入操作不能满足（对于TCP套接字即至少有一个字节数的数据可读，对于UDP套接字既有一个完整的数据报可读），相应的返回一个EWOULDBLOCK的错误。

(2)输出的操作，包括write、writev、send、sendto和sendmsg共5个函数。对于一个TCP套接字我们已经在2.11中说过，内核将从应用进程的缓冲区到套接字的发送缓冲区复制数据，对于阻塞的套接字，如果发送缓冲区没有空间那么进程就会投入睡眠，直到有空间为止

对于一个非阻塞的TCP套接字，如果发送缓冲区中根本没有空间，输出缓冲区中将会立即返回一个EWOULDBLOCK错误。如果发送缓冲区中有一些空间，返回值是内核能够复制到该缓冲区的字节数，这个字节数也成为不足计数。

我们还在2-11中说过，UDP套接字不存在真正的发送缓冲区。内核只是复制应用进程数据并且把它沿协议栈向下传送。渐次冠以UDP首部和IP首部。因此对一个阻塞的UDP套接字。因此一个阻塞的UDP套接字并不会和TCP一样被阻塞，而可能因为其他原因被阻塞

(3)即将外来的连接，即accept函数，如果对一个阻塞的套接字调用accept函数，并且没有新的连接到达，调用进程将被投入睡眠。

如果一个非阻塞的套接字调用accept函数，并且没有新的连接到达，accept就会立即返回一个EWOULDBLOCK错误。

(4)发起外出连接，即用于TCP的连接函数connect函数。（回顾以下我们知道的connect函数，他同样适用于UDP但是他不会真的发起连接，只是在内核中固定ip和端口号，TCP的建立涉及到一个三路握手的过程，而且connect函数一直等到客户收到对于自己的SYN的ACK后才返回，这意味着TCP每一次调用connect都会阻塞一个RTT的时间）。

如果对于一个非阻塞的TCP套接字调用connect，并且连接不能被立即建立，那么连接的建立可以被照样发起（譬如送出TCP三路握手的第一个分组），不过返回一个EINPROGRESS错误。注意这个错误不等同于上面的非阻塞错误。另外有一些连接可以被立即建立，通常发生在客户和主机位于同一台主机上面的情况，因此即使对于一个非阻塞的connect，我们也得预备connect成功返回的情况发生

对于不能满足的非阻塞模式，系统会返回EAGAIN错误

####16.2 非阻塞读和写：str_cli函数

select 可以使IO非阻塞，但是如果系统缓冲区已满，或者网速比较慢，照样会将writen阻塞。

不幸运的是非阻塞的io使操作更加复杂化了

我们维护着两个缓冲区：to容纳从标准输入到服务器去的数据，fr容纳自服务器到标准输出的数据。

代码例子：

	#include "unp.h"

	void str_cli(FILE* fp,int sockfd)
	{
		int maxfdpl,val,stdineof;

		ssize_t n,nwritten;

		fd_set rset,wset;

		char to[MAXLINE],char fr[MAXLINE];

		char *toiptr,*tooptr,*friptr,*froptr;

		fcntl(sockfd,F_SETFL,0);

		val = fcntl(STDIN_FILENO,F_GETFL,0);

		fcntl(STDOUT_FILENO,F_SETFL,val|O_NONBLOCK);

		toiptr = tooptr = to;
		friptr = froptr = fr;

		stdineof = 0;

		maxfdpl = max(max(STDIN_FILENO,STDOUT_FILENO),sockfd) + 1;

		for(;;){
			FD_ZERO(&rset);
			FD_ZERO(&wset);

			if(stdineof == 0 && toipr < &to[MAXLINE])
				FD_SET(STDIN_FILENO,&rset);

			if(friptr != tioptr)
				FD_SET(sockfd,&wset);

			if(froptr != friptr)
				FD_SET(STDOUT_FILENO,&wset);

			select(maxfdpl,&rset,&wset,NULL,NULL);

			if(FD_SET(STDIN_FILENO,toiptr,&to[MAXLINE])){
				if(n = read(STDIN_FILENO,toiptr,&to[MAXLINE] - toiptr) < 0){
					if(errno != EWOULDBLOCK)
					{
						err_sys("read error on stdin");
					}
				}else if(n == 0)
				{
					fprintf(stderr,"%s:EOF on stdin\n",gf_time());

					toipr += n;
					FD_SET(sockfd,&wset);
				}else{
					fprintf(stderr,"%s:read %d bytes from stdin\n",gf_time(),n);
					toiptr += n;
					FD_SET(sockfd,&wset);
				}
			}

			if(FD_SET(sockfd,&rset))
			{
				if((n = read(sockfd,friptr,&fr[MAXLINE] - friptr)) < 0)	
				{
					if(errno != EWOULDBLOCK)
					{
						err_sys("read error on socket");
					}
				}else if(n == 0)
				{
					fprintf(stderr,"%s:EOF on socket\n",gf_time());

					if(stdineof)
						return;
					else
						err_quit("str_cli:server terminated prematurely");
				}else{
					fprintf(stderr,"%s:read %d bytes from socket\n",gf_time(),n);

					friptr += n;
					FD_SET(STDOUT_FILENO,&wset);
				}
			}

			if(FD_ISSET(sockfd,&wset) && (( n = toiptr - tooptr) > 0)){
				if(( nwriten = write(STDOUT_FILENO,froptr,n)) < 0){
					if(errno != EWOULDBLOCK)
					{
						err_sys("write error to stdout");
					}
				}else{
					fprintf(stderr,"%s:wrote %d bytes to stdout \n",gf_time(),nwritten);

					froptr += nwritten;

					if(froptr == friptr)
						froptr = friptr =fr;
				}
			}


			if(FD_ISSET(sockfd,&wset) && ((n = toiptr - tooptr) > 0)){
				if((nwritten = write(sockfd,tooptr,n) < 0))
				{
					if(errno != EWOULDBLOCK)
					{
						err_sys("write error to socket");
					}
				}else{
					fprintf(stderr,"%s:wrote %d bytes to socket\n",gf_time(),nwritten);
				
					tooptr += nwritten;

					if(tooptr == toiptr){
						toiptr = tooptr = to;
						if(stdineof)
						{
							shutdown(sockfd,SHUT_WR);
						}
					}
				}
			}
		}
	}

####16.3非阻塞connect

当在一个非阻塞的TCP上调用connect的时候，conncet会立即返回一个EINPROGRESS错误，不过已经发起的三路握手仍然在继续。我们紧接着使用select检测这个链接成功或失败的已经建立起来的条件。非阻塞的connect有三个用途。

1）我们可以把三次握手叠加在其他处理上。完成一个connect要花一个RTT的时间，而RTT的波动范围十分大，从几十到几百毫秒甚至到几秒，这段时间内也许有其他程序等待着我们执行

2）我们可以使用这个技术建立多个链接。这个用途已经随着web浏览器流行了起来。

3）既然使用select等待链接建立，我们可以给select一个时间限制，让我们可以缩短connect的超时时间。应用程序有时需要一个更加短暂的时间，实现方法之一就是使用非阻塞connect。

####16.4非阻塞获取客户时间

设置套接字为非阻塞。

发起非阻塞conncect。期望的错误是EINPROGRESS表示链接建立已经启动但是没有完成

调用select等待套接字变为可读或者可写

如果select返回0那么链接就超时了，我们需要关闭套接字否则三路握手还在继续。

如果描述符可读或者可写，我们调用getsockopt获取套接字待处理的错误。如果处理成功，这个值将为0.如果链接建立错误，但是不同操作系统可能存在移植性问题，这个值就是errno的错误值。但是我们在最后判断了error是否为0，所以这个问题得到了解决。	

关闭非阻塞状态并且返回，如果getsockopt返回的error为非0关闭套接字返回-1，否则返回0

检测链接建立成功的方法：

1）调用getpeername代替getsockopt，如果返回ENOTCONN错误的返回，那么连接建立失败，我们必须在接下来使用getsockopt的SO_ERROR选项获取待处理的错误码。

2）以值为0的长度参数调用read。如果read失败，那么connect已经失败，read返回的errno给处理失败的原因否则read返回0，那么就是连接成功了。

3）再次connect，如果成功返回的错误是EISCONN，也就是说第一次连接成功了。

不幸的是非阻塞connect是最难移植的。


被终端的connect

被中断的connect：

对于一个非阻塞的套接字，如果其上的connect调用在TCP三路握手王城前被终端，将会发生什么呢。如果不由内核重启将返回EINTR。我们不能再次吊起connect。这样会返回EADDRINUSE的错误。

这种情况下我们只能调用select，链接建立成功时但会可写条件，建立失败则返回又可读又可写的条件。

	//
	// Created by root on 18-12-30.
	//
	#include <sys/socket.h>
	#include <fcntl.h>
	#include <errno.h>
	#include <unistd.h>
	#include <stdio.h>
	#include <stdlib.h>
	int connect_nonb(int sockfd,const struct sockaddr *saptr,socklen_t salen,int nsec)
	{
	    int flags,n,error;
	    socklen_t len;

	    fd_set rset;

	    fd_set wset;

	    struct timeval tval;

	    flags = fcntl(sockfd,F_GETFL,0);

	    //set no block
	    fcntl(sockfd,F_SETFL,flags|O_NONBLOCK);

	    error = 0;

	    if((n=connect(sockfd,saptr,salen) < 0))
	    {
		if(errno != EINPROGRESS)
		{
		    return -1;
		}
	    }

	    if( n == 0)
		goto done;

	    FD_ZERO(&rset);
	    FD_SET(sockfd,&rset);
	    wset = rset;

	    tval.tv_sec = nsec;
	    tval.tv_usec = 0;

	    if((n = select(sockfd+1,&rset,&wset,NULL,&tval)) == 0)
	    {
		close(sockfd);
		errno = ETIMEDOUT;
		return  -1;
	    }

	    if(FD_ISSET(sockfd,&rset) || FD_ISSET(sockfd,&wset)){
		len = sizeof(error);

		if(getsockopt(sockfd,SOL_SOCKET,SO_ERROR,&error,&len) < 0)
		    return  -1;
	    }else{
		printf("select error:socket not set");
		exit(-1);
	    }

	    done:
		fcntl(sockfd,F_SETFL,flags);//erstore

		if(error)
		{
		    close(sockfd);
		    errno = ETIMEDOUT;
		    return  -1;
		}

		return 0;
	}

	int main()
	{
	    int scokfd = socket(AF_INET,SOCK_STREAM,0);
	}
	


####16.5 非阻塞的connect web程序

非阻塞的connect的实现例子出自Netscape的web程序.客户先建立一个于某个web程序相关联的http连接。该主页往往含有多个对于其网页的引用。该主页往往含有多个其他的web页，以此来取代只有一个web页的串行获取手段。展示了并行建立一个或者多个连接的例子。最左边串行执行已经有的三个连接。假设第一个消耗十个单位时间，第二个消耗15个单位时间，第三个消耗4个单位时间，总计消耗29个单位时间。

中间的情形并行执行2个连接。在时刻0启动前2个连接，其中一个结束了执行第三个连接。启动时间差不多削减了一半从29变成了15

处理web客户的时候，第一个连接独立执行，来自该连接的数据含有多个引用，随后用于访问这些引用的多个连接则并行执行。

为了进一步优化连接执行序列，客户可以在第一个连接尚未完成之前就开始分析从中陆续返回的数据，以便熟悉其中含有的引用，并尽快启动相应的额外连接。

既然准备同事处理多个connect连接那就不可以再使用connect_nonb了，因为它必须要等待才可以返回，我们必须自行管理这些连接。

web.h

	#include <sys/socket.h>
	#include <sys/types.h>
	#include <stdio.h>
	#include <stdlib.h>
	#define MAXFILES 20
	#include <netdb.h>
	#include <string.h>
	#include <errno.h>
	#include <arpa/inet.h>
	#include <unistd.h>
	#include <fcntl.h>
	#define SERV "80"
	#define MAXLINE 255
	#define min(a,b) (a<b) ? a : b

	struct file{
	    char *f_name;
	    char* f_host;
	    int f_fd;
	    int f_flags;
	}file[MAXFILES];

	#define F_CONNECTING 1
	#define F_READING 2
	#define F_DONE 4

	#define GET_CMD  "GET %s HTTP/1.0\r\n\r\n"

	int nconn,nfiles,nlefttoconn,nlefttoread,maxfd;

	fd_set rset,wset;

	void home_page(const char*,const char *);

	void start_connect(struct file*);

	void write_get_cmd(struct file*);

	char* getHostIp(char* host);

	int connect_nonb(int sockfd,const struct sockaddr *saptr,socklen_t salen,int nsec);
	
web.c

	//
	// Created by root on 19-1-6.
	//
	#include "web.h"

	void sys_err(char* string){
	    printf("%s\n",string);
	    exit(0);
	}

	int main(int argc,char** argv)
	{
	    int i,fd,n,maxconn,flags,error;
	    char buf[MAXLINE];

	    fd_set rs,ws;


	    if(argc < 5)
	    {
		sys_err("usage:web <#conn> <hostname> <homepage> <file1>...");
	    }

	    maxconn = atoi(argv[1]);
	    nfiles = min(argc-4,MAXFILES);


	    for(i=0;i<nfiles;i++)
	    {
		printf("%d\n",i);
		file[i].f_host = argv[2];
		file[i].f_flags = 0;
		file[i].f_name = argv[i]+4;
	    }

	    printf("%s\n",argv[2]);
	    home_page(argv[2],argv[3]);

	    FD_ZERO(&rset);
	    FD_ZERO(&wset);

	    maxfd = -1;

	    nlefttoconn = nlefttoread = nfiles;

	    nconn = 0;

	    while(nlefttoread > 0)
	    {

		while(nconn < maxconn && nlefttoconn > 0)
		{
		    for(i=0;i<nfiles;i++)
		    {
		        if(file[i].f_flags == 0)
		        {
		            break;
		        }
		    }
		    if(i==nfiles)
		    {
		        printf("nlefttoconn = %d but nothing found\n",nlefttoconn);
		        exit(0);
		    }

		    start_connect(&file[i]);
		    nconn++;
		    printf("nlefttoconn:%d\n",nlefttoconn);
		    nlefttoconn--;
		}

		rs = rset;

		ws = wset;

		n = select(maxfd+1,&rs,&ws,NULL,NULL);


		for(i =0;i<nfiles;i++)
		{
		    flags = file[i].f_flags;

		    if(flags == 0 || flags&F_DONE)
		    {
		        continue;
		    }

		    fd = file[i].f_fd;

		    if(flags & F_CONNECTING && (FD_ISSET(fd,&rs) || FD_ISSET(fd,&ws)))
		    {
		        n = sizeof(error);

		        if(getsockopt(file[i].f_fd,SOL_SOCKET,SO_ERROR,&error,&n) < 0)
		        {
		            sys_err("connect error");
		        }

		        printf("%d connect ok\n",file[i].f_fd);
		        FD_CLR(file[i].f_fd,&wset);
		        n = snprintf(buf,sizeof(buf),GET_CMD,file[i].f_name);
		        write(fd,buf,n);
		        file[i].f_flags = F_READING;
		    }else if(flags & F_READING && FD_ISSET(fd,&rs))
		    {
		        while(1) {
		            if ((n = (int) read(file[i].f_fd, buf, sizeof(buf))) == 0) {

		                break;
		            } else {
		                if(errno == EINTR)
		                {
		                    continue;
		                }
		                printf("read:%s\n", buf);
		            }
		        }

		        printf("end of file pn %s\n",file[i].f_name);
		        close(fd);
		        file[i].f_flags = F_DONE;
		        FD_CLR(fd,&rset);
		        nconn--;
		        nlefttoread--;
		    }
		}
	    }

	}

noblock_connect.c

	//
	// Created by root on 19-1-6.
	//
	#include "web.h"
	int connect_nonb(int sockfd,const struct sockaddr *saptr,socklen_t salen,int nsec)
	{
	    int flags,n,error;
	    socklen_t len;

	    fd_set rset;

	    fd_set wset;

	    struct timeval tval;

	    flags = fcntl(sockfd,F_GETFL,0);

	    //set no block
	    fcntl(sockfd,F_SETFL,flags|O_NONBLOCK);

	    error = 0;

	    if((n=connect(sockfd,saptr,salen) < 0))
	    {
		if(errno != EINPROGRESS)
		{
		    return -1;
		}
	    }

	    if( n == 0)
		goto done;

	    FD_ZERO(&rset);
	    FD_SET(sockfd,&rset);
	    wset = rset;

	    tval.tv_sec = nsec;
	    tval.tv_usec = 0;

	    if((n = select(sockfd+1,&rset,&wset,NULL,&tval)) == 0)
	    {
		close(sockfd);
		errno = ETIMEDOUT;
		return  -1;
	    }

	    if(FD_ISSET(sockfd,&rset) || FD_ISSET(sockfd,&wset)){
		len = sizeof(error);

		if(getsockopt(sockfd,SOL_SOCKET,SO_ERROR,&error,&len) < 0)
		    return  -1;
	    }else{
		printf("select error:socket not set");
		exit(-1);
	    }

	    done:
	    fcntl(sockfd,F_SETFL,flags);//erstore

	    if(error)
	    {
		close(sockfd);
		errno = ETIMEDOUT;
		return  -1;
	    }

	    return 0;
	}

	void start_connect(struct file* web_file)
	{
	    char* web_host;
	    int fd,connect_result,flags;
	    web_host = getHostIp(web_file->f_host);
	    fd = socket(AF_INET,SOCK_STREAM,0);
	    struct sockaddr_in addr;
	    addr.sin_family = AF_INET;
	    addr.sin_port = htons(80);
	    inet_pton(AF_INET,web_host,&addr.sin_addr);
	    socklen_t len = sizeof(addr);
	    flags =  fcntl(fd,F_GETFL,0);
	    fcntl(fd,F_SETFL,flags|O_NONBLOCK);
	    connect_result = connect(fd,(struct sockaddr*)&addr,len);
	    if(connect_result != 0)
	    {
		if(errno != EINPROGRESS){
		    printf("connect error:%d\n",connect_result);
		    exit(0);
		}

	    }

	    FD_SET(fd,&rset);
	    FD_SET(fd,&wset);

	    if(fd > maxfd)
		maxfd = fd;

	    web_file->f_flags = F_CONNECTING;
	    web_file->f_fd = fd;

	}

home_page.c

	//
	// Created by root on 19-1-6.
	//
	#include "web.h"



	char* getHostIp(char* host)
	{
	    printf("host:%s\n",host);
	    struct hostent* host_struct;
	    host_struct = gethostbyname(host);
	    static char str[MAXLINE];
	    while(*host_struct->h_addr_list)
	    {
		inet_ntop(AF_INET,*host_struct->h_addr_list,&str,MAXLINE);

		*host_struct->h_addr_list++;
	    }

	    return str;
	}

	void home_page(const char *host,const char *fname)
	{

	    int fd,n;
	    char line[MAXLINE];
	    char* result = getHostIp(host);
	    printf("%s\n",result);
	    fd = socket(AF_INET,SOCK_STREAM,0);

	    struct sockaddr_in addr;
	    bzero(&addr,sizeof(addr));

	    addr.sin_port = htons(80);
	    addr.sin_family = AF_INET;
	    inet_pton(AF_INET,result,&addr.sin_addr);

	//    addr.sin_addr.s_addr = htonl(result);

	    socklen_t len = sizeof(addr);
	    int connresult = connect_nonb(fd,(struct sockaddr*)&addr,len,5);

	    if(connresult != 0)
	    {
		printf("socket connect errno:%d\n",errno);
		exit(0);
	    }

	//    strcpy(line,);
	    n = snprintf(line,sizeof(line),GET_CMD,fname);
	    printf("content:%s\n",line);
	    write(fd,line,n);
	    char buf[2048];
	    for(;;)
	    {
		if((n = read(fd,buf,2048)) == 0)
		{
		    break;
		}else{
		    printf("read %s\n",buf);
		}
	    }
	    printf("end of file on home page");
	    close(fd);
	}

			


	