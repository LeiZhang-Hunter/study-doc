#第8章 基本UDP套接字编程

####1.概述

UDP与TCP存在着不同，主要表现在传输层上的差异，其中原因在于两个协议传输层之间的不同，UDP是无连接 不可靠的数据报传输协议，TCP是全面可靠的套接字流。

UDP客户与服务不需要建立连接，而是只管使用sendto函数给服务器发送数据报，其中必须指定目的地址作为参数。同样的服务器不接受来自客户的连接而是只管使用recvfrom函数，等待来自某个客户的数据报到达。recvfrom将一道返回客户的地址协议，因此服务器可以把响应发送给正确的客户。

我们将使用两个全新的函数进行数据收发,recvfrom和sendTo。

####2.recvfrom 和 sendTo

这两个函数类似于read和write函数

    #include <sys/socket.h>
    
    size_t recvfrom(int sockfd,void* buff,size_t nbytes,int flags,struct sockaddr *from,socklen_t* addrlen);

    size_t sendto(int sockfd,void *buff,size_t nbytes,int flags,const struct sockaddr *to,socklen_t *addrlen);

前三个参数 是 描述符 输入输出的缓冲区  以及字节数

其中sendto参数中的to指向一个将由该函数在返回时填写数据报的发送者的协议地址的套接字结构体，而套接字地址的字节数则放到addrlen里（值-结果参数）。

在UDP情况下发送0字节，将会由IPV4(20字节)或者IPV6（40字节）的IP首部，和一个8字节的UDP首部，recvfrom返回0是可以接受的，它并不像TCP上返回0就标识连接已经断开。既然UDP是无连接的那么就没有关闭连接这一说。

recvfrom 和 sendto 同样可以用在TCP

####3.udp回射客户程序main函数

例子:

    #include "unp.h"

    int main(int argc,char **argv)
    {
        int sockfd;
        struct sockaddr_in servaddr,cliaddr;

        sockfd = socket(AF_INET,SOCK_DGRAM,0);

        bzero(servaddr,sizeof(servaddr));

        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port = htons(SERV_PORT);

        socklen_t serv_struct_len = sizeof(servaddr);

        bind(sockfd,(struct sockaddr*)servaddr,&serv_struct_len);
        
    }

总结 由于udp本身并不是一个面向对象的连接，所以根本不存在listen 维护套接字队列，和accept返回已经准备就绪的描述符一说，只需要调用recvfrom和sendto进行数据收发就可以了。

####4.udp 服务器回射客户端程序dg_echo 函数

给出dg_echo函数

    #include "unp.h"

    void dg_echo(int sockfd,(struct sockaddr*)cliaddr,socklen_t clilen)
    {
        int n;
        socklen_t len;
        char mesg[MAXLINE];

        for(;;)
        {
            len = clilen;
            n = recvfrom(sockfd,mesg,MAXLINE,0,cliaddr,&len);

            sendto(sockfd,mesg,n,0,cliaddr,len);
        }
    }

尽管这个函数十分的简单，但是我们要考虑的问题依然十分的多，该函数没有类似EOF的东西，永远不会处于停止的状态

大多数UDP都是迭代的 没有并发一说，所以不存在像TCP那样处理一个请求需要建立一个进程。


####5.udp 回射客户程序main 函数

    #include "unp.h"
    
    int main(int argc,char **argv)
    {
        int sockfd;
        sockfd = socket(AF_INET,SOCK_DGRAM,0);
        struct sockaddr_in serveraddr;
        bzero(&serveraddr,sizeof(serveraddr));
    
        serveraddr.sin_family = AF_INET;
        serveraddr.sin_port = htons(SERV_PORT);
        serveraddr.sin_addr.s_addr = inet_addr(LOCAL);
        
        dgcli(stdin,sockfd,(struct sockaddr *)serveraddr,sizeof(serveraddr));
    
        exit(0);
    }

####6.UDP回射客户程序：dg_cli函数

    #include "unp.h"

    void dg_cli(FILE* fp,int sockfd,const struct sockaddr * servaddr,socklen_t servlen)
    {
        int n;
        char sendline[MAXLINE],recvline[MAXLINE+1];

        while(fgets(sendline,sizeof(sendline),stdin) != NULL)
        {
            sendto(sockfd,sendline,sizeof(sendline),0,servaddr,servlen);

            n = recvfrom(sockfd,recvline,sizeof(recvline),0,NULL,NULL);
            
            recvline[n] = 0;
            fputs(recvline,sizeof(recvline),stdout);
        }
    }

recvfrom 最后两个参数为NULL这代表我们并不关心是谁发过来的消息，但是这样存在一个风险：任何进程不论是在于本客户进程相同的主机上还是在不同的主机上，都可以对客户的ip和端口发送数据报，这些数据报将被客户机读入，并且认为是和服务器主机得应答

####7.数据报的丢失

    我的UDP客户/服务器例子是不可靠的。如果一个客户数据报丢失（比如说客户机与服务机某个路由被丢弃），客户将永远阻塞于dg_cli函数中的recvfrom的调用，等待一个永远不会到达的服务器应答。类似的如果客户数据到达服务器，但是服务器应答丢失，客户将永远阻塞于recvfrom的调用。防止永久阻塞的方法是给recvfrom设置一个超时时间。

####8.8验证接收到的响应

指导客户机的临时端口任何主机都可以对客户机进行消息发送，我们的解决方案是修改recvfrom的调用以返回数据发送的ip和端口号，保留来自数据所发往服务器的应答，而忽略任何其他的数据报

    #include "unp.h"

    void dg_cli(FILE* fp,int sockfd,const struct * pservaddr,socklen_t serclen)
    {
        int n;
        char sendline[MAXLINE],recvline[MAXLINE+1];
        socklen_t len;
        struct sockaddr *preply_addr;
        
        preply_addr = malloc(servlen);
        
        while(fgets(sendline,MAXLINE,fp) != NULL)
        {
            sendto(sockfd,sendline,strlen(sendline),0,pservaddr,servlen);
            
            len = servlen;
            
            n = recvfrom(sockfd,recvline,MAXLINE,0,preplay_addr,&len);
            
            if(len != servlen || memcmp(pservaddr,preplay_addr,len) !=0)
            {
                printf("replay from %s (ignored)\n",sock_ntop(preply_addr,len));
                continue;
            }
        }
    } 
    
####9.服务器进程没有运行

如果服务器没有启动那么我们不管发送什么服务器都会被阻塞在recvfrom，等待一个永远不会出现的服务器应答

使用tcpdump我们很容易发现，客户机在通往服务器发送udp协议之前，需要一次ARP的请求和应答的转换，然而在后面我们会看到ICMP消息。不过这个ICMP的错误不返回给客户进程。客户永远阻塞于recvfrom的调用。ICMPV6也有端口不可达错误。该错误是一个异步错误，由sendto引起来的，udp的输出返回成功，仅仅是表示接口在输出队列具有存放形成ip的数据报空间。该ICMP错误是在4ms之后才返回。

这个ICMP出错消息包含引起错误的ip首部和所有udp首部或tcp首部，以便确定由哪个套接字引发这个错误，但是内核如何把错误消息返回给进程，recvfrom可以返回信息仅仅有errno 不会有ip和端口号。因此做出决定只有在进程已经将UDP套接字链接到恰恰一个对端后，这些异步错误才返回跟进程

####10.udp例子小节

服务器udp 建立过程:

指定服务器的众所周知的端口=》指定服务器众所周知的ip=》数据链路收发数据

客户端建立过程

由udp客户选择临时端口=》由ip选择的客户的ip地址=》数据链路收发数据

客户必须要调用sendto调用指定服务器的ip地址和端口号，一般来说客户的ip地址和端口号是由内核自动选择，客户也可以调用bind指定他们。客户临时端口是由调用sendto后一次性选定的，不能改变。然而客户的ip地址可以随客户发送的每个UDP数据报而变动。如果客户主机是多宿主的，客户可能在两个目的地之间交替选择，其中一个由左边的数据链路外出，另一个由右边的数据链路外出。在这种最坏情况下，由内核基于外出数据选择的客户ip地址将随每个数据报而改变。

服务器想从IP的数据报上获取至少四条信息：源ip地址，目的地ip地址，源端口号和目的端口号

服务器总是能很便捷的访问已连接套接字这四个方面信息，而且这四个值在连接的整个声明周期保持不变。然而对于UDP套接字，只可以通过设置IP_RECVDSTADDR套接字选项，然后调用recvmsg获取注意不再是recvfrom了，由于UDP是无连接的，因此目的ip地址可随发送到服务器的每个数据报发生改变。UDP服务也可接收目的地址为服务器主机的某个广播或者多播的数据报。

从服务的角度总结UDP客户/服务器

TCP服务器:

来源ip               accept

来源端口              accept

目的地ip              getsockname

目的地端口             getsockname

UDP:服务器

来源ip               recv

来源端口              accept

目的地ip              recvmsg

目的地端口             getsockname

####11.UDP connect函数

除非UDP已经连接否则错误信息并不会返回，我们确实可以给UDP调用connect函数，但是udp没有三次握手，内核只是检查是否存在已经知道的错误，记录对端的ip地址和端口号，然后立即返回给调用进程。

有了这个能力我们也必须要区分：

未连接UDP套接字，新创建的udp 套接字默认如此

未连接UDP套接字，对UDP调用connect结果

对于已经连接的UDP套接字和默认的未连接UDP套接字相比 发生了三个变化：

1）我们再也不能给输出操作指定目的ip和端口号了，我们不可以使用sendto了，而是需要受用write或者send，写到已连接的udp套接字上，写上去的任何内容都是由connect指定协议

2）我们不必使用recvfrom以获取数据报的发送者，而是使用read，recv或者是recvmsg。在一个已经连接的udp套接字上，内核返回的输入数据只有connect指定的协议，这样就限制了一个udp只能于一个对端进行数据交换

3）由已经连接的UDP错误会返回给他们所在的进程，而未连接的不接收任何异步错误

套接字类型:

TCP套接字:

write或send:可以

不指定目的地址的sendto:可以

指定目的地址的sendto:EISCONN

UDP套接字,已连接:

write或send:可以

不指定目的地址的sendto:可以

指定目的地址的sendto:EISCONN

UDP套接字未连接:

write或send:EDESTADDREQ

不指定目的地址的sendto:EDESTADDREQ

指定目的地址的sendto:可以


应用进程首先调用connect指定对端的ip地址和端口号，然后使用read与write和对端进行数据交换

来自任何其他ip地址或端口的数据报不投递给这个已连接的套接字，因为它们要么源ip地址要么源udp端口不予该connect的协议地址相互匹配。这些数据报可能投递给同一个主机上的其他某个UDP套接字。如果没有相匹配的其他套接字，UDP将会丢弃他们并生成ICMP端口不可达错误。

作为小节我们可以说udp客户进程或服务器进程只在使用自己的udp与确定唯一的端口进行通信的时候，才可以调用connect。调用connect通常是udp客户，不过有些网络应用中udp服务器可能与客户长时间通信，也可以调用connect。

DNS提供了一个例子

比如resolv.conf，如果是一个dns服务器主机，那么客户端可以调用connect如果是多个客户端则不能调用connect

######11.1.给一个UDP套接字多次调用connect

udp套接字可处于以下两个目的再次调用connect

1）指定新的ip和端口

2）重连已经断开的套接字

对于一个已经断开的套接字我们再次调用connect时候需要把套接字协议簇设置为AF_UNSPEC.这么做可能会返回一个EAFNOSUPPORT错误，不过没有关系。使套接字断开是在已连接的UDP上调用connect的进程。

######11.2性能

当应用进程在一个未连接的UDP套接字上调用sendto的时候，内核会暂时连接该套接字，发送数据报，然后断开连接。在一个未连接的UDP套接字上，给两个数据报调用sendto函数于是涉及内核执行下列6个步骤：

    连接套接字：
    输出第一个数据报
    断开套接字
    连接套接字：
    输出第二个数据报
    断开套接字

当应用进程知道自己要给同一个目的地地址发送多个数据报的时候，显示连接效率更高执行效率如下:

    连接套接字：
    输出第一个数据报
    输出第二个数据报

总结 ：
也就是说connect 后调用效率更高因为少了 连接 和 close 过程，
当套接字断开再次connect的时候要把协议簇设置为AF_UNSPEC

####12.dg_cli函数
    
    #include "unp.h"
    
    void dg_cli(FILE *fp,int sockfd,const struct sockaddr* pservaddr,socklen_t servlen)
    {
        int n;
        char sendline[MAXLINE],recvline[MAXLINE+1];
        connect(sockfd,(struct sockaddr*)pservaddr,&servlen);

        while(fgets(sendline,MAXLINE,fp) != NULL)
        {
            write(sockfd,sendline,strlen(sendline));

            n = read(sockfd,recvline,MAXLINE);

            recvline[n] = 0;

            fputs(recvline,MAXLINE,stdout);
        }
        
    }
    
    
    所修改的是我们调用了connect，并且用read和write取代了sendto和recvfrom，这个函数不查看传递给connect的套接字地址结构内容，因此是与协议无关，因此main函数不会发生改变
    
udp缺少流量控制
    
现在我们查看无任何流量控制的udp对数据报传输的影响。我们把dg_cli修改为发送固定数目，并且不在读取标准输入。它写2000个1400字节的UDP数据发送给服务器

    #include "unp.h"
    
    #define NDG 2000
    #define DGLEN 1400
    
    void dg_cli(FILE* fp,int sockfd,const SA* pservaddr,socklen_t servlen)
    {
        int i;
        char sendline[DGLEN];
        
        for(i=0;i<ndg;i++)
        {
            sendto(sockfd,sendline.DGLEN,0,pservaddr,servlen);
        }
    }
    
服务端接收并且计数，但是不再把数据回射给客户端，当我们终止服务器时候，会显示接收的数目并且终止

我们把客户运行在RS6000上，服务器运行在一台较差电脑上发现丢包率是98%，出现这种原因是因为接收缓冲区已经满了

我们可以通过so_recvbuf来修改套接字缓冲区不过这不可能从根本上解决问题


####14.UDP外出接口的确定

已连接的UDP套接字用于某个特定目的的外出接口，是connect应用udp套接字时候的一个副作用造成的：内核选择本地ip地址。这个本地的ip地址搜索路由表得到的外出接口，然后选该ip地址而选定。

一个简单应用程序来获取本地ip地址和端口号并且显示输出，在udp套接字上调用connect它并不会给对端发送任何消息，只是一个本地操作，保存对端的ip地址和端口号。在一个未绑定的UDP套接字上调用connect会生成一个临时端口。

    #include "unp.h"
    
    int main()
    {
        int sockfd;
        socklen_t len;
        struct sockaddr_in cliaddr,servaddr;
        
        sockfd = sock(AF_INET,SOCK_DGRAM,0);
        
        bzero(&servaddr,sizeof(servaddr));
        
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(SERV_PORT);
        inet_pton(AF_INET,argv[1],&servaddr.sin_addr));
    
        connect(sockfd,(SA*)&servaddr,sizeof(servaddr));
        
        len = sizeof(cliaddr);
        
        getsockname(sockfd,(SA *)&cliaddr,&len);
        
        printf("local address %s:%d\n",sock_ntop((SA*)&cliaddr,len)));
    
        exit(0);
    }

####15.使用select函数的TCP和UDP回射服务程序

	#include "unp.h"

	int main(int argc,char** argv){
		int listenfd,connfd,udpfd,maxfdpl;

		char mesg[MAXLINE];

		pid_t childpid;

		fd_set rset;

		ssize_t n;

		socklen_t len;

		const int on = 1;

		struct sockaddr_in cliaddr,servaddr;

		void sig_child(int);

		//create tcp
		listenfd = socket(AF_INET,SOCK_STREAM,0);

		bzero(&servaddr,sizeof(servaddr));

		servaddr.sin_family = AF_INET;
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		servaddr.sin_port = htons(SERV_PORT);

		bind(sockfd,&servaddr,sizeof(servaddr));

		//create udp
		udpfd = socket(AF_INET,SOCK_DGRAM,0);

		bzero(&servaddr,sizeof(servaddr));

		servaddr.sin_family = AF_INET;
		servaddr.sin_port = htons(SERV_PORT);
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

		bind(udpfd,&servaddr,sizeof(servaddr));

		signal(SIGCHILD,sig_child);

		FD_ZERO(&rset);

		maxfdpl = max(listenfd,udpfd)+1;

		for(;;)
		{
			FD_SET(listenfd,&rset);
			FD_SET(udpfd,&rset);

			if((nready = select(maxfdpl,&rset,NULL,NULL,NULL)) < 0){
				if(errno == EINTR)
				{
					continue;
				}else{
					sys_err("select error\n");
				}
			}

			if(FD_ISSET(listenfd,&rser)){
				len = sizeof(cliaddr);
				connfd = accept(listenfd,(SA*)&cliaddr,&len);

				if(childpid = fork() == 0)
				{
					close(fd);
					str_echo(connfd);
					exit(0);
				}
				close(fd);
			}

			if(FD_ISSET(udpfd,&rset))	
			{
				len = sizeof(cliaddr);
				n = recvfrom(uudpfd,mesg,MAXLINE,0,(SA*)&cliaddr,len);
				sendto(ufpfd,mesg,n,0,(SA*)&cliaddr,len);
			}
		}
	}