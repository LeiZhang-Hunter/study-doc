#第十五章UNIX域协议

####15.1概述

UNIX域协议并不是一个实际的协议族，而是在单个主机上执行客户和服务通讯的一种方案。unix域协议可以被视为IPC通信方式之一。

UNIX域提供两类套接字：字节流套接字和数据报套接字。

1）unix域套接字往往比通信两端位于同一个主机的TCP套接字快出一倍。如果服务器于客户机处于同一个主机，客户就打开服务器的unix域字节流连接，否则打开一个服务器的TCP连接。

2）unix域套接字可以在同一个主机不同进程之间传递描述符

3）unix域套接字较新实现把客户的凭证提供给服务器，从而提供额外的安全检测措施。（用户ID和组ID）

####15.1UNIX域套接字地址结构

	struct sockaddr_un{
		sa_family_t sun_family;
		char sun_path[104];
	}
	


sun_path数组中的路径名必须以空字符结尾。实现提供SUN_LEN宏以一个指向sockaddr_un结构的指针为参数并返回该结构的长度，其中包含路径名中非空的字节数。未指定地址通过以空字符串作为路径名只是，也就是一个sun_path[0]值为0的地址结构。

unix 域套接字的bind 调用

	#include "unp.h"

	#include "common.h"

	int main(int argc, char** argv)
	{
		int sockfd;
	
		int len;
		sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);
	
		struct sockaddr_un serveraddr,cliaddr;
	
		serveraddr.sun_family = AF_LOCAL;
	
		strncpy(serveraddr.sun_path,argv[1],sizeof(serveraddr.sun_path)-1);
	
		unlink(argv[1]);
	
		bind(sockfd,(struct sockaddr*)&serveraddr,sizeof(serveraddr));
	
		len = sizeof(cliaddr);
	
		getsockname(sockfd,(struct sockaddr *)&cliaddr,&len);
	
		printf("biund name = %s,returned len = %d\n", cliaddr.sun_path, len);
	
		exit(0);
	}

1.我们需要删除路径文件，如果文件存在那么会删除文件，如果文件存在的话绑定unix域套接字会失败，如果不存在unlink会出现一个令我们忽略的错误。

2.bind 然后getsockname

我们使用strncpy复制命令行参数，以避免路径名过长导致地址溢出。既然我们已经把地址结构初始化为0，并且从sun_path中减去1，可以肯定该路径名以空字符串结尾。之后调用bind 再使用getsockname取得绑定的路径名字并且显示结果。

php的实现:

	<?php
	$dir ='/ourc/fpm2';
	$socket = socket_create(AF_UNIX,SOCK_STREAM,0);
	unlink($dir);
	socket_bind($socket,$dir);
	$addr = [];
	socket_getsockname($socket,$addr);
	var_dump($addr);

####15.3 socketpair 函数

socketpair函数创建两个随后连接起来的套接字。本函数仅仅适应于unix域套接字。

	#include <sys/socket.h>

	int socketpair(int family,int type,int protocol,int sockfd[2]);

family参数必须为AF_LOCAL,protocol参数必须为0.type参数既可以是SOCK_STREAM。要创建的两个套接字描述符作为sockfd[0]和sockfd[1]返回。

类似管道的例子:

	/*
	 *进程双向通信
	 */
	#include<stdio.h>  
	#include<string.h>  
	#include<sys/types.h>  
	#include<stdlib.h>  
	#include<unistd.h>  
	#include<sys/socket.h>  
	
	
	int main()
	{
		int sv[2];    //一对无名的套接字描述符  
		if (socketpair(AF_LOCAL, SOCK_STREAM, 0, sv) < 0)   //成功返回零 失败返回-1  
		{
			perror("socketpair");
			return 0;
		}
	
		pid_t id = fork();        //fork出子进程  
		if (id == 0)               //孩子  
		{
			//close(sv[0]); //在子进程中关闭读  
			close(sv[1]); //在子进程中关闭读  
	
			const char* msg = "i am children\n";
			char buf[1024];
			while (1)
			{
				// write(sv[1],msg,strlen(msg));  
				write(sv[0], msg, strlen(msg));
				sleep(1);
	
				//ssize_t _s = read(sv[1],buf,sizeof(buf)-1);  
				ssize_t _s = read(sv[0], buf, sizeof(buf) - 1);
				if (_s > 0)
				{
					buf[_s] = '\0';
					printf("children say : %s\n", buf);
				}
			}
		}
		else   //父亲  
		{
			//close(sv[1]);//关闭写端口  
			close(sv[0]);//关闭写端口  
			const char* msg = "i am father\n";
			char buf[1024];
			while (1)
			{
				//ssize_t _s = read(sv[0],buf,sizeof(buf)-1);  
				ssize_t _s = read(sv[1], buf, sizeof(buf) - 1);
				if (_s > 0)
				{
					buf[_s] = '\0';
					printf("father say : %s\n", buf);
					sleep(1);
				}
				// write(sv[0],msg,strlen(msg));  
				write(sv[1], msg, strlen(msg));
			}
		}
		return 0;
	}

####15.4 套接字函数

1）由bind 创建路径名默认访问权限应为0777（属主用户、组用户和其他用户都可读可写并且可执行），并且按照当前umask值进行修改。

2）unix域套接字路径名应该是一个绝对路径，而不是一个相对的路径名。避免使用者后者的原因是它依赖于调用者当前的工作目录。也就是说服务器捆绑一个相对路径名字，客户就得在与服务器相同的目录中才能成功调用connect 和 sendto。

3）在connect调用中指定路径名必须是一个当前绑定在某个打开的

5)Unix域字节流套接字与tcp一样都有一个无边界	的字节流接口

6）如果unix某个域套接字队列已满会返回ECONNREFUSED错误，不会像TCP一样忽略掉信号

7）unix域数据报类似与UDP套接字，提供一个不可靠的数据报服务。

8）在一个未绑定的unix域套接字上发送数据报不会自动给这个套接字绑定一个路径名，这一点不等同于UDP套接字：在一个未绑定的UDP套接字上发送UDP数据报导致这个套接字绑定一个临时端口。这就意味着除非数据报发送端已经绑定了一个路径名到他的套接字，否则接收端无法回应答数据报。类似的某个unix域套接字的connect调用不会给本套接字绑定一个路径名，这一点不同于TCP和UDP。

####15.5 unix域字节流客户服务程序

与tcp 类似，服务端程序：

//
// Created by root on 18-11-10.
//

	#include <sys/un.h>
	#include <stdlib.h>
	#include <stdio.h>
	#include <fcntl.h>
	#include <sys/socket.h>
	#include <stdint.h>
	#include <signal.h>
	#include <wait.h>
	#include <errno.h>
	#include <zconf.h>
	void sigchild(int signo)
	{
	    pid_t pid;
	    int stat;

	    while ((pid = waitpid(-1,&stat,WNOHANG)) > 0)
		printf("child %d terminated\n",pid);

	    return;
	}

	int main()
	{
	    int listenfd;

	    listenfd = socket(AF_LOCAL,SOCK_STREAM,0);

	    struct sockaddr_un serveraddr,cliaddr;

	    bzero(&serveraddr,sizeof(serveraddr));

	    serveraddr.sun_family = AF_LOCAL;

	    strcpy(serveraddr.sun_path,"/usr/local/soft/default/unix_socket");

	    unlink("/usr/local/soft/default/unix_socket");
	    bind(listenfd,(struct sockaddr*)&serveraddr,sizeof(serveraddr));

	    listen(listenfd,50);

	    signal(SIGCHLD,sigchild);

	    bzero(&cliaddr, sizeof(cliaddr));

	    pid_t  child_pid;

	    int clien;
	    int connfd;
	    for(;;)
	    {
		clien = sizeof(cliaddr);
		if((connfd = accept(listenfd,(struct sockaddr*)&cliaddr,&clien)) < 0)
		{
		    if(errno == EINTR)
		    {
		        continue;
		    } else{
		        printf("accept error\n");
		        exit(0);
		    }



		}else{
		    if((child_pid = fork()) == 0)
		    {
		        close(listenfd);
		        char buf[100];
		        if(read(connfd,buf,sizeof(buf)) > 0)
		        {
		            printf("%s\n",buf);
		            exit(-1);
		        }
		    }
		}

		close(connfd);

	    }

	}
	
客户端程序：

	//
	// Created by root on 18-12-22.
	//

	#include <sys/un.h>
	#include <stdlib.h>
	#include <stdio.h>
	#include <fcntl.h>
	#include <sys/socket.h>
	#include <stdint.h>
	#include <signal.h>
	#include <wait.h>
	#include <errno.h>
	#include <zconf.h>

	int main(int argc,char** argv)
	{
	    int sockfd;
	    struct sockaddr_un servaddr;

	    sockfd = socket(AF_LOCAL,SOCK_STREAM,0);

	    bzero(&servaddr, sizeof(servaddr));


	    strcpy(servaddr.sun_path,"/usr/local/soft/default/unix_socket");

	    servaddr.sun_family = AF_LOCAL;

	    connect(sockfd,(struct sockaddr*)&servaddr,sizeof(servaddr));

	    char buf[40];
	    bzero(&buf,sizeof(buf));
	    strcpy(buf,"hello world");
	    write(sockfd,buf,sizeof(buf));
	}
	
####15.6 数据报服务客户程序
	
与TCp一样不再写了，只不过是把SOCK_STREAM变为SOSCK_DRGM


####15.7 描述符传递

当考虑一个进程的描述符传递到另一个进程的时候，我们通常会想到

fork调用返回后，子进程共享父进程所有打开的描述符

exec调用执行之后，所有描述符通常保持打开状态不变

进程打卡一个描述符，调用fork，然后父进程关闭这个描述符，子进程处理这个描述符。然而有的时候我们想让子进程把秒杀父母传递到父进程

当前的unix系统提供了用于从一个进程向任意一个其他进程传递一个打开的描述符的方法，也就是说两个进程不需要存在亲源关系，比如父子进程关系。这种技术首先在两个进程之间创建一个unix域套接字，然后使用sendmsg跨这个套接字发送一个特殊消息。这个消息由内核来专门处理，会把打开的描述符从发送进程传递到接收进程。

两个进程之间描述符的传递步骤如下：

1)创建一个字节流或数据报的套接字

如果目标是父子进程，那么可以用socketpair创建一个可用于在父子进程之间交换的描述符流管道。

如果进程之间没有血缘关系，那么服务进程必须要创建一个域服务器套接字，bind一个路径名到这个套接字，来允许该进程connect到这个套接字。然后客户可以向发送一个打开某个描述符的请求，服务器再把这个描述符通过域套接字传递回客户。客户和服务之间也可以通过unix域数据报套接字，不过这么做没什么好处。而且数据报还有丢失的可能性。

2)发送进程通过调用函数unix任意函数打开一个描述符。这些函数有open，pipe，mkfifo，socket，accept，可以在进程之间传递的秒杀父母不限制类型，这就是我们称他为描述符传递而不是文件描述符传递的原因。

3）发送创建一个msghdr结构体，其中含有描述符。POSIX规定描述符作为辅助数据（msghdr结构的msg_control成员）发送，不过老的实现是通过msg_accrights成员。发送进程通过调用sendmsg跨来自步骤1的unix域套接字发送该描述符。至此我们说我们的描述符在飞行中。即使发送进程在调用sendmsg后接收进程用recvmsg之前关闭了描述符，对于接收进程依然保持打开状态。发送一个描述符会让这个描述符的引用计数+1。

4)接收进程在调用recvmsg接收来自步骤1的描述符。这个描述符在接收进程中的秒描述符不同于他在发送中的描述符是正常的。传递一个描述符并不是传递一个符号，而是涉及到在接收进程中创建一个新的符号，而这个新描述符和发送进程中飞行前的那个描述符指向内核中相同的文件表项。

客户和服务器之间必须存在某种协议，以便描述符在接收进程中预先知道何时接收。如果接收进程在调用recvmsg没有用于分配接收描述符的空间，而且之前一个描述符已经被传递而且被等待 读取，这个早先传递的描述符就会被关闭。另外在接收描述符的recvmsg中，应该避免使用MSG_PEEK,否则后果不可预料。


######描述符传递的小例子

	#include "unp.h"

	int my_open(const char*,int);

	int main(int argc,char** argv)
	{
		int fd,n;
		char buff[BUFSIZE];

		if(argc != 2)
			err_quit("usage:mycat <pathname>");

		if((fd = my_open(argv[1],O_RDONLY)) < 0)
			err_sys("cannot open %s",argv[1]);

		while((n = read(fd,buff,BUFFSIZE)))
			write(STDOUT_FILENO,buff,n);

		exit(0);
	}


	int my_open(const char *pathname,int mode)
	{
		int fd,sockfd[2],status;

		pid_t childpid;

		char c,argsockfd[10],argmode[10];

		socketpair(AF_LOCAL,SOCK_STREAM,0,sockfd);

		if((childpid = fork()) == 0){
			close(sockfd[0]);
			snprintf(argsockfd,sizeof(argsockfd),"%d",sockfd[1]);

			snprintf(argmode,sizeof(argmode),"%d",sockfd[1]);

			execl("./openfile","openfile",argsockfd,pathname,argmode,(char *)NULL);

			err_sys("execl error");
		}

		close(sockfd[1]);

		waitpid(childpid,&status,0);

		if(WIFEXITED(status) == 0)
			err_quit("child did not terminate");

		if((status = WEXITSTATUS(status)) == 0)
			read_fd(sockfd[0],&c,1,&fd);
		else{
			errno = status;
			fd = -1;
		}

		close(soclfd[0]);
		return fd;
	}

1)首先我们使用socketpair创建了一个流管道，返回了两个描述符：sockfd[0]和sockfd[1].

2)调用fork，子进程然后关闭流管道的一端。流管道的另一端的描述符格式化输出到argsockfd字符数组，打开方式格式化输出到argmode字符数组，这里使用snprintf格式化进行输出是因为exec的参数必须是字符串。子进程随后调用execl执行openfile程序。这个函数不会返回，除非他出现错误。一旦成功，openfile程序打开所请求文件时碰到一个错误，它将以相应的errno值作为退出状态终止自身。

3）父进程等待子进程
父进程关闭流管道的另一端并且使用waitpid等待子进程终止（防止出现僵尸进程）。子进程的终止状态再status之中返回，我们首先应该检查是否是正常终止（也就是说不是被某一个信号终止），如果是正常终止接着调用WEXITSTATUS宏把终止状态切换成退出状态，退出状态的值再0~255之间。我们马上会看到在打开请求文件的时候会碰到一个错误，他将会以对应的errno值作为退出状态来终止自身

readfd函数的使用：

	zsize_t read_fd(int fd,void* ptr,size_t nbytes,int* recvfd)
	{
		struct msghdr msg;
		struct iovec iov[1];
		ssize_t n;

	#ifdef HAVE_MSGHDR_MSG_CONTROL
		union{
			struct cmsghdr cm;
			char control[CMSG_SPACE(sizeof(int))];
		} control_un;

		msg.msg_control = control_un.control;
		msg.msg_controllen = sizeof(control_un.control);
	#else
		int newfd;
		msg.msg_accrights = (caddr_t)&newfd;
		msg.msg_accrightslen  = sizeof(int);
	#endif

		msg.msg_name = NULL;
		msg.namelen = 0;

		iov[0].iov_base = ptr;
		iov[0].iov_len = nbytes;

		msg.msg_iov = iov;
		msg.msg_iovlen = 1;

		if((n = recvmsg(fd,&msg,0)) < = 0)
		{
			return n;
		}

	#ifdef HAVE_MSGHDR_MSG_CONTROL
		if((cmptr = CMSG_FIRSTHDR(&msg)) != NULL && cmptr->cmsg_len == CMSG_LEN(sizeof(int)))
		{
			if(cmptr->cmsg_level != SOL_SOCKET)
				err_quit("control level != SOL_SOCKET");

			if(cmptr->cmsg_type != SCM_RIGHTRS)
				err_quit("control level != SCM_RIGHTRS");

			*recvfd = *((int *)CMSG_DATA(cmptr));
		}else{
			*recvfd = -1
		}
	#else
		if(msg.msg_accrightslen == sizeof(int))
			*recvfd = newfd;
		else
			*recvfd = -1;
	#endif	
		return n;
	}

openfile程序

	#include "unp.h"

	int main(int argc,char** argv)
	{
		int fd;

		if(argc != 4)
			err_quit("openfile <sockfd*> <filename> <mode>");

		if((fd = open(argv[2],atoi(argv[3]))) < 0)
			exit((errno > 0) ? errno : 255);

		if(write_fd(atoi(argv[1]),"",1,fd) < 0)
			exit((errno > 0) ? errno : 255);

		exit(0);
	}

writefd函数
	
	#include "common.h"
	#define    HAVE_MSGHDR_MSG_CONTROL 

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
	
		return sendmsg(fd,&msg,0);
	}

write_fd函数把描述符传递回父进程之后，本进程立即终止。本章早先说过，发送进程可以不等落地就关闭已经传递的描述符，因为内核指导描述符在飞行仲，从而为接收进程保持打开状态。

自己写的实验例子:

msg_main.c 

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

msg.c:

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

运行：
	
	[root@localhost ourc]# ./msg_main /ourc/a.php 
	/ourc/a.php
	argsockfd n:4
	argmode n:0
	argmode n:/ourc/a.php
	result:0
	55555555
	status n:0
	recvmsg main fd:3


php 字节流 域套接字 跨进程传递描述符代码案例

服务端:

	<?php
	ini_set("display_errors",true);
	//
	$socket = socket_create(AF_UNIX,SOCK_STREAM,0);
	$dir = "/root/fpm";
	unlink($dir);
	socket_bind($socket,$dir);
	socket_listen($socket,100);
	
	while(1)
	{
	    $connfd = socket_accept($socket);
	    var_dump($connfd);
	    $data = ["controllen" => socket_cmsg_space(SOL_SOCKET, SCM_RIGHTS, 3)];
	    $msg = socket_recvmsg($connfd,$data,0);
	    var_dump($data);
	}

客户端:


	<?php

	ini_set("display_errors", true);
	$socket = socket_create(AF_UNIX, SOCK_STREAM, 0);
	$dir = "/root/fpm";
	$result = socket_connect($socket, $dir);
	
	var_dump($result);
	$fp = fopen("/root/common.h", "r");
	var_dump($fp);
	$r = socket_sendmsg($socket, [
	        "iov" => [" "],
	        "control" =>
	        [
	            [
	                "level" => SOL_SOCKET,
	                "type" => SCM_RIGHTS,
	                "data" => [$fp]
	            ]
	        ]
	    ]
	    , 0);

php 数据报流 域套接字 跨进程传递描述符代码案例

	<?php
	$s = socket_create(AF_UNIX, SOCK_DGRAM, 0) or die("err");
	unlink($dir);
	$br = socket_bind($s, $dir) or die("err");
	
	$dir = "/root/fpm";
	
	while(1)
	{
	    $data = ["name" => [], "buffer_size" => 2000, "controllen" => socket_cmsg_space(SOL_SOCKET, SCM_RIGHTS, 3)];
	    $result = socket_recvmsg($s, $data, 0);
	    if($result)
	    {
	        var_dump($data);
	    }
	}

客户端:
	<?php
	ini_set("display_errors", true);
	$socket = socket_create(AF_UNIX, SOCK_STREAM, 0);
	$dir = "/root/fpm";
	$result = socket_connect($socket, $dir);
	
	var_dump($result);
	$fp = fopen("/root/common.h", "r");
	var_dump($fp);
	$r = socket_sendmsg($socket, [
	        "iov" => [" "],
	        "control" =>
	        [
	            [
	                "level" => SOL_SOCKET,
	                "type" => SCM_RIGHTS,
	                "data" => [$fp]
	            ]
	        ]
	    ]
	    , 0);

####15.8 接收者发送凭证

图14-13 展示可以通过unix域套接字作为辅助数据传递的另一种数据是用户凭证。作为辅助数据的凭证其具体封方式和发送方式往往特定于操作系统。本节讨论FREEBSD的凭证传递

FREEBSD使用在头文件<sys/socket.h>中定义的cmsgcred传递结构凭证。

	struct	cmsgcred{
		pid_t cmcred_pid;
		uid_t cmcred_uid;
		uid_t cmcred_euid;
		god_t cmcred_gid;
		short cncred_ngroups;
		gid_t cmcred_groups;
	}

CMGROUP_MAX通常为16.cmcred_ngroups总是为1，而且cmcred_ngroups数组的第一个元素是有效组ID。接收函数recvmsg只需要提供一个足以存放凭证的辅助数据空间就可以。sendmsg发送辅助数据的时候必须作为一个辅助数据包含cmsgcred结构才会随数据传递凭证。

	#include "unp.h"

	#define CONTROL_LEN (sizeof(struct cmsghdr) + sizeof(struct cmsgcred))

	ssize_t read_cred(int,void*,size_t,struct cmsgcred*);

	ssize_t read_cred(int fd,void* ptr,size_t nbytes,struct cmsgcred *cmsgcredptr)
	{
		struct cmsgcred cmsgcredptr;
		struct msghdr msg;
		struct iovec iov[1];
		char control[CONTROL_LEN];
		int n;

		msg.msg_name = NULL;
		msg.msg_namelen = 0;

		iov[0].iov_base = ptr;
		iov[0].iov_len = nbytes;

		msg.msg_iov = iov;
		msg.msg_iovlen = 1;

		msg.msg_control = control;
		msg.msg_controllen = sizeof(control);
		msg.msg_flags = 0;

		if((n = recvmsg(fd,&msg,0)) < 0)
			return n;

		cmsgcredptr->cmcred_ngroups = 0

		if(cmsgcredptr && msg.msg_controller > 0)
		{
			struct cmsghdr *cmptr = (struct cmsghdr *)control;

			if(cmptr->cmsg_len < CONTROL_LEN)
			{
				err_quit("control length %d",cmptr->cmsg_len);
			}

			if(cmptr->level != SOL_SOCKET)
			{
				err_quit("control level != SOL_SOCKET");
			}

			if(cmptr->cmsg_type != SCM_CREDS){
				err_quit("control level != SCM_CREDS");
			}

			memcpy(cmsgcredptr,CMSG_DATA(cmptr),sizeof(struct cmsgcred));
		} 


		return n;
	}

小结：

UNIX域套接字是客户和服务器同在一个主机上的IPC方法之一，与其他IPC相比，UNIX域套接字的优势体现在API几乎等同于网络客户和服务的api。与服务客户在同一台主机上相比它的优势集中体现在性能上。他们的主要区别是域套接字必须要绑定到一个路径上来使UDP有发送到达的目的地。在同一个主机上传递描述符的技术十分有用。