#第四章 管道和FIFO

这一个章节主要描述了管道和FIFO的创建和使用。我们使用一个简单的文件服务器例子，同事查看一些客户-服务程序的设计问题：IPC通道需要量、迭代服务器与并发服务器、字节流与消息接口.

###4.2 一个简单的客户和服务端例子

我们来说明管道、FIFO和system v 消息队列

路径名 -------》标准输入-------》客户------------》路径名-------》服务器
文件内容或出错消息《------- 标准输出《-------客户《-------文件内容或出错消息《-------

客户通过读入一个路径名字，把它写入ipc通道。服务器从这个ipc通道读出这个路径名字，并且尝试打开其文件来读.如果服务器可以打开这个文件就读出文件内容，并且写入ipc通道，作为客户的响应；否则返回一个出错消息，通过ipc管道读出响应，并且把它写给标准输出。如果服务器无法读这个文件，那么客户读出的响应是一个出错消息.

	#include <unistd.h>
	
	int pipe(int fd[2]);
	
这个函数通过两个文件描述符fd[0]和fd[1].前者fd[0]打开来读，后者fd[1]打开来写.

宏S_ISFIFO可用于确定一个描述符或者是文件是管道还是FIFO.它唯一的参数是stat结构的st_mode成员，计算结果或者为真，或者为假(0).对于管道来说，这个stat结构是由fstat函数填写的。对于FIFO来说，这个结构是由fstat、lstat或者stat函数填写的.

管道是在单个进程中创建的。却很少在单个进程中使用，管道的经典用途是在两个不通的进程之间通讯，首先由一个进程创建一个管道，然后fork派生一个自身的副本.

接着父进程关闭管道的读出端，子进程关闭同一个管道的写入端.这就在父子进程中形成一个单向的数据流.

我们在某个unix shell中输入下面一个命令

输入 who | sort | lp 这个管道都是半双工，即单向的，只提供一个方向的数据流.当需要一个双向数据流的时候，我们创建两个管道，每个方向一个实际步骤如下：

1）创建管道1和管道2（fd1[0]和fd1[1]） 和管道2 （fd2[0]和fd2[1]）

2)  fork

3) 父进程关闭管道1的读出端（fd1[0]）

4）父进程关闭管道2的写入端(fd2[1]);

5)子进程关闭管道1的写入端(fd1[1])

6)子进程关闭管道1的写入端(fd1[0])

	#include <net/if_arp.h>
	#include <netinet/ether.h>
	#include <netinet/in.h>
	#include <stdlib.h>
	#include <stdio.h>
	#include <strings.h>
	#include <string.h>
	#include <net/if.h>
	#include <sys/socket.h>
	#include <arpa/inet.h>
	#include <unistd.h>
	#include <sys/ioctl.h>
	#include <errno.h>
	#include <ucontext.h>
	#include <zconf.h>
	#include <stdbool.h>
	#include <sys/wait.h>
	#include <fcntl.h>

	#define SLEN 20
	#define MAXLEN 50

	void server(int readfd,int writefd);

	void client(int readfd,int writefd);

	int main(int argc,char **argv)
	{
	    int pipe1[2],pipe2[2];
	    pid_t pid;

	    //创建管道
	    pipe(pipe1);

	    pipe(pipe2);

	    if((pid = fork()) == 0)
	    {
		close(pipe1[1]);
		close(pipe2[0]);

		server(pipe1[0],pipe2[1]);
		exit(0);
	    }

	    close(pipe1[0]);
	    close(pipe2[1]);

	    client(pipe2[0],pipe1[1]);

	    waitpid(pid,NULL,0);
	}

	void client(int readfd,int writefd)
	{
	    size_t len;
	    ssize_t n;
	    char buff[MAXLEN];

	    fgets(buff,MAXLEN,stdin);

	    len = strlen(buff);

	    if(buff[len-1] == '\n')
		len--;

	    write(writefd,buff,len);

	    while((n = read(readfd,buff,MAXLEN)) > 0) {
		write(STDOUT_FILENO, buff, (size_t)n);
	    }
	}

	void server(int readfd,int writefd)
	{
	    int fd;
	    ssize_t n;

	    char buff[MAXLEN+1];

	    if((n = read(readfd,buff,MAXLEN)) == 0)
	    {
		printf("end of file while reading pathname\n");
		exit(-1);
	    }

	    buff[n] = '\0';

	    if((fd = open(buff,O_RDONLY)) < 0)
	    {
		snprintf(buff+n,sizeof(buff)-n,": can not open,%s\n",strerror(errno));

		n = strlen(buff);

		write(writefd,buff,n);
	    }else{
		while((n = read(fd,buff,MAXLEN)) > 0)
		    write(writefd,buff,n);

		close(fd);
	    }
	}

总结:
管道是单向的，但是我们可以创建两个管道，实现进程之间的双向通讯，但是管道还是单向的。


####4.4全双工管道的实现

fd[1]------->write------->(半双工管道)---->read------->fd[0];

全双攻管道可能实现成这样的。他的隐含的意思是：整个管道只存在一个缓冲区，（在任意一个描述符上）写入管道的任何数据都添加到缓冲区末尾，（在任意一个描述符上）从管道读出的都是自缓冲区开头的数据.

写入的数据放到缓冲区末尾，读出缓冲区的数据到缓冲区开头

这种实现存在的问题在像图A-29这样的程序中变得更加明显。我们需要双向通讯，但是所需要的是两个独立的数据流，每个方向一个，如果不是这样，当一个进程往该全双工管道上写入数据，过后再对这个管道调用read的时候，有可能读入回刚才写入的数据.

正确的全双工管道是由两个半双工管道构成的。写入fd[1]的数据只能从fd[0]读出，写入fd[0]的数据只能从fd[1]读出

####4.5 popen 和 pclose

popen执行一端指令并且获取他的返回



	FILE* fp = popen("ls /home/zhanglei","r");

    char res[MAXLEN];

    while(fgets(res,sizeof(res),fp))
    {
	fputs(res,stdout);
    }

    pclose(fp);
    return 0;
    

    
   我们使用pclose 来关闭popen的描述符
   
   
   ####4.6 FIFO
   
   管道是没有名字的，只能用在有血缘关系之间的进程，没有血缘关系的进程要使用有名字的ipc方式.
   
   fifo类似于管道，他是一个半双工的与文件路径相关联.
   
   	#include <sys/types.h>
   	#include <sys/stat.h>
   	
   	int mkfifo(const char *pathname,mode_t mode);
   	
   mkfifo第二个参数类似于一个权限位
   
   mkfifo已经隐藏了O_CREAT 和 O_EXCL.也就是说要么他创建一个新的fifo，要么返回一个EEXIST的错误。如果不希望创建一个新的fifo，那么改为open而不是mkfifo.应该先调用mkfifo，检查是否是EEXIST错误，如果说存在的话在调用open打开
   
   
   有血缘关系的例子:
   
	#define FIFO1   "/tmp/fifo.1"
	#define FIFO2   "/tmp/fifo.2"
	#define FILE_MODE 0666

	int main(int argc,char **argv)
	{
	    int readfd,writefd;
	    pid_t childpid;

	    if((mkfifo(FIFO1,FILE_MODE)) < 0 && (errno != EEXIST))
	    {
		printf("create fifo1 error\n");
		exit(0);
	    }

	    if((mkfifo(FIFO2,FILE_MODE)) < 0 && (errno != EEXIST))
	    {
		printf("create fifo2 error\n");
		exit(0);
	    }

	    if((childpid = fork()) == 0)
	    {
		readfd = open(FIFO1,O_RDONLY,0);
		writefd = open(FIFO2,O_WRONLY,0);
		server(readfd,writefd);
		exit(0);
	    }

	    writefd = open(FIFO1,O_WRONLY,0);
	    readfd = open(FIFO2,O_RDONLY,0);

	    client(readfd,writefd);

	    waitpid(childpid,NULL,0);

	    close(readfd);
	    close(writefd);

	    unlink(FIFO1);
	    unlink(FIFO2);
	    exit(0);
	    return 0;
	}

	void client(int readfd,int writefd)
	{
	    size_t len;
	    ssize_t n;
	    char buff[MAXLEN];

	    fgets(buff,MAXLEN,stdin);

	    len = strlen(buff);

	    if(buff[len-1] == '\n')
		len--;

	    write(writefd,buff,len);

	    while((n = read(readfd,buff,MAXLEN)) > 0) {
		write(STDOUT_FILENO, buff, (size_t)n);
	    }
	}

	void server(int readfd,int writefd)
	{
	    int fd;
	    ssize_t n;

	    char buff[MAXLEN+1];

	    if((n = read(readfd,buff,MAXLEN)) == 0)
	    {
		printf("end of file while reading pathname\n");
		exit(-1);
	    }

	    buff[n] = '\0';

	    printf("%s\n",buff);

	    if((fd = open(buff,O_RDONLY)) < 0)
	    {
		snprintf(buff+n,sizeof(buff)-n,": can not open,%s\n",strerror(errno));

		n = strlen(buff);

		write(writefd,buff,n);
	    }else{
		while((n = read(fd,buff,MAXLEN)) > 0)
		    write(writefd,buff,n);

		close(fd);
	    }
	}
	
	
	
在无血缘的情况下将client和server拆开以及代码拆开依旧可以运行.

####4.7 管道和fifo的额外属性.

设置成非阻塞有两种方法 第一是调用open设置O_NONBLOCK，第二是调用fcntl

当write的时候如果小于等于PIPE_BUF是原子的，如果大于则不是院子的

如果write时候管道已经满了会返回EAGAIN错误，在阻塞模式下会投入休眠等待.	

如果调用进程没有捕获SIGPIPE,默认会终止进程，如果捕获了，会返回EPIPE错误码

####4.10 字节流和消息

由于是字节流，发送数据快的时候会粘包注意做分包处理即可.

common.h 文件代码如下:

	#include <net/if_arp.h>
	#include <netinet/ether.h>
	#include <netinet/in.h>
	#include <stdlib.h>
	#include <stdio.h>
	#include <strings.h>
	#include <string.h>
	#include <net/if.h>
	#include <sys/socket.h>
	#include <arpa/inet.h>
	#include <unistd.h>
	#include <sys/ioctl.h>
	#include <errno.h>
	#include <ucontext.h>
	#include <zconf.h>
	#include <stdbool.h>
	#include <sys/wait.h>
	#include <fcntl.h>
	#include <pwd.h>
	#include <sys/stat.h>
	//
	// Created by zhanglei on 19-4-16.
	//

	#ifndef SFF_COMMON_H
	#define SFF_COMMON_H

	#endif //SFF_COMMON_H


mesg.h 文件代码如下:

	//
	// Created by zhanglei on 19-4-16.
	//
	#include "common.h"
	#ifndef SFF_MESG_H
	#define SFF_MESG_H

	#endif //SFF_MESG_H

	#define MAXMESGDATA (PIPE_BUF-2*sizeof(long))

	#define MESGHDRSIZE (sizeof(struct mymesg) - MAXMESGDATA)

	struct mymesg{
	    long mesg_len;
	    long mesg_type;
	    char mesg_data[MAXMESGDATA];
	};

	ssize_t mesg_send(int,struct mymesg*);
	void Mesg_send(int,struct mymesg*);
	ssize_t mesg_recv(int,struct mymesg*);
	ssize_t Mesg_recv(int,struct mymesg*);


test.c 文件代码如下:

	#define SLEN 20
	#define MAXLEN 50

	#include "mesg.h"
	#define FIFO1   "/tmp/fifo.1"
	#define FIFO2   "/tmp/fifo.2"
	#define FILE_MODE 0666
	void client(int readfd,int writefd);
	void server(int readfd,int writefd);

	int main(int argc,char **argv)
	{
	    int readfd,writefd;
	    pid_t childpid;

	    if((mkfifo(FIFO1,FILE_MODE)) < 0 && (errno != EEXIST))
	    {
		printf("create fifo1 error\n");
		exit(0);
	    }

	    if((mkfifo(FIFO2,FILE_MODE)) < 0 && (errno != EEXIST))
	    {
		printf("create fifo2 error\n");
		exit(0);
	    }

	    if((childpid = fork()) == 0)
	    {
		readfd = open(FIFO1,O_RDONLY,0);
		writefd = open(FIFO2,O_WRONLY,0);
		server(readfd,writefd);
		exit(0);
	    }

	    writefd = open(FIFO1,O_WRONLY,0);
	    readfd = open(FIFO2,O_RDONLY,0);

	    client(readfd,writefd);

	    waitpid(childpid,NULL,0);

	    close(readfd);
	    close(writefd);

	    unlink(FIFO1);
	    unlink(FIFO2);
	    exit(0);
	    return 0;
	}

	ssize_t mesg_send(int fd,struct mymesg *mptr)
	{
	    return (write(fd,mptr,MESGHDRSIZE + mptr->mesg_len));
	}

	ssize_t mesg_recv(int fd, struct mymesg *mptr)
	{
	    size_t len;
	    ssize_t n;

	    if((n = read(fd,mptr,MESGHDRSIZE)) == 0)
	    {
		return 0;
	    }else if(n != MESGHDRSIZE)
	    {
		printf("message header :expected %ld,got %ld,MESGHDRSIZE",MESGHDRSIZE,n);
	    }
	}

	void client(int readfd,int writefd)
	{
	    size_t len;
	    ssize_t n;
	    struct mymesg mesg;

	    fgets(mesg.mesg_data,MAXMESGDATA,stdin);

	    len = strlen(mesg.mesg_data);

	    if(mesg.mesg_data[len-1] == '\n')
		len--;

	    mesg.mesg_len = len;
	    mesg.mesg_type = 1;

	    mesg_send(writefd,&mesg);

	    while((n = mesg_recv(readfd,&mesg)) > 0)
	    {
		write(STDOUT_FILENO,mesg.mesg_data,n);
	    }
	}

	void server(int readfd,int writefd)
	{
	    FILE *fp;
	    ssize_t n;
	    struct mymesg mesg;
	    mesg.mesg_type = 1;

	    if((n = mesg_recv(readfd,&mesg)) == 0)
	    {
		printf("pathname missing");
	    }

	    mesg.mesg_data[n] = '\0';

	    if((fp = fopen(mesg.mesg_data,"r")) == NULL){

		snprintf(mesg.mesg_data + n, sizeof(mesg.mesg_data) - n,"can't open,%s\n",strerror(errno));

		mesg.mesg_len = strlen(mesg.mesg_data);

		mesg_send(writefd,&mesg);

	    }else{

		while(fgets(mesg.mesg_data,MAXMESGDATA,fp) != NULL)
		{
		    mesg.mesg_len = strlen(mesg.mesg_data);
		    mesg_send(writefd,&mesg);
		}

	    }

	    mesg.mesg_len = 0;
	    mesg_send(writefd,&mesg);
	}


####4.11 管道和FIFO限制.

系统加载管道和FIFO的唯一限制为：

OPEN_MAX 一个进程在任意时刻打开的最大描述符数目

PIPE_BUFF 可原子的往一个管道或FIFO里写入的最大数据量.

OPEN_MAX的值我们可以通过ulimit指令或者sysconf里修改，或者调用setrlimit函数修改.

于是PIPE_BUF的值在运行时通过调用pathconf或者fpathconf取得

示例代码：

	#include <net/if_arp.h>
	#include <netinet/ether.h>
	#include <netinet/in.h>
	#include <stdlib.h>
	#include <stdio.h>
	#include <strings.h>
	#include <string.h>
	#include <net/if.h>
	#include <sys/socket.h>
	#include <arpa/inet.h>
	#include <unistd.h>
	#include <sys/ioctl.h>
	#include <errno.h>
	#include <ucontext.h>
	#include <zconf.h>
	#include <stdbool.h>
	#include <sys/wait.h>
	#include <fcntl.h>
	#include <pwd.h>
	#include <sys/stat.h>

	#define SLEN 20
	#define MAXLEN 50



	int main(int argc,char **argv)
	{
	    if(argc != 2)
	    {
		printf("usage:pipeconf <pathname>");
	    }

	    printf("PIPE_BUF = %ld,OPEN_MAX = %ld\n",pathconf(argv[1],_PC_PIPE_BUF),sysconf(_SC_OPEN_MAX));
	}


OPEN_MAX可以使用ulimit -ns来做修改.