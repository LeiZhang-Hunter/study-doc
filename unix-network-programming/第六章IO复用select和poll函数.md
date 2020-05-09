#第六章I/O复用:select和poll 函数

####1.IO复用经典应用场合

1.客户端处理多个套接字时，必须使用IO复用

2.如果一个TCP服务器既需要处理监听套接字，又需要处理已经连接的IO套接字，那么他就必须要使用IO复用

3.如果一个服务器既要处理TCP又要处理UDP，一般就要使用IO复用

4.一个服务器要处理多个服务或者多个协议，一般要使用IO复用

####2.IO复用的整体回顾

unix下的5种IO模式:

阻塞IO

非阻塞IO

IO复用

信号驱动式

异步IO

######2.1阻塞式IO

前5章所有的例子记本都是阻塞IO，所有的套接字都是阻塞的比如说 read  write

本书中的recvfrom视为内核调用，一般会在应用程序空间切换至内核空间再切换回来

######2.2非阻塞式IO

进程把一个套接字设置成为非阻塞在通知内核：当所请求的IO操作非得把本进程投入睡眠才能完成时，不要把本进程投入睡眠，而是返回一个错误，这种情况下一般要使用轮询（polling），一直等待到收到数据，不过这种情况一般会很吃CPU。

######2.3 IO复用模型

调用poll和select这两个参数，阻塞在这两个系统调用的某一个，而不是阻塞在真正的IO系统上

select->调用系统内核（无数据报等待）->有数据报返回

我们阻塞与select 进行等待，等待数据报套接字变成可读的。当select返回套接字可读这一个条件的时候，我们调用recvfrom把数据报复制到应用进程的缓冲区。

IO复用可能优势并不是很明显，事实上使用select需要两个系统来调用，IO复用还有劣势，但是使用select却可以监控多个文件描述符。

与IO复用密切相关的还有另一种模型就是在多线程中使用阻塞IO，与上述模型十分相似，但他并没有使用select阻塞在多文件描述符上

######2.4信号驱动IO模型

我们也可以用信号，让内核在描述符就绪的时候就时序发送SIGIO信号来通知我们，我们称这种模型为信号驱动式IO

我们首先开启套接字的信号驱动式IO功能，并且通过sigaction系统调用一个系统信号处理函数。被系统立即返回，我们的进程继续工作，也就是说他没有被阻塞。当数据报准备好读取的时候，内核就会为该进程产生一个SIGIO信号，我们随后即可在信号处理函数中调用recvfrom读取数据报，并且通知主循环已经准备好处理，或让它读取数据报

######2.5异步IO	模型

异步IO由POSIX规范定义。这些函数的工作机制是告知内核执行某个操作，并让内核在整个操作完成后通知我们。与信号驱动的主要区别是信号驱动系统是用信号通知我们，而异步是内核通知我们执行的完成

####3.select 函数

该函数允许进程指定内核等待多个事件中的任何一个发生，并在只有一个或者多个事件发生或经历一段指定的时间后才唤醒他

作为一个例子我们可以调用select函数，告知内核仅在下列情况下才返回

	集合{1,4,5}中的任何描述符准备好读
	集合{2,7}任何描述符准备好写
	集合{1,4}任何描述符有异常条件待处理
	已经历了10.2秒

也就是说，我们调用select 告知内核对哪些描述符感兴趣以及等待多长时间。我们感兴趣的描述符不仅局限与套接字，任何描述符都可以使用select来做测试

	#include <sys/select.h>
	#include <sys/time.h>

	int select(int maxfdp1,fd_set *readset,fd_set *writeset,fd_set *exceptset,const struct timeval *timeout);

timeval 结构体:

	struct timeval{
		long tv_sec; /*seconds*/
		long tv_usec;  /* microseconds */
	} 

这个参数有一下三种可能:

1)永远等待下去：仅在有一个描述符准备好IO才会返回，为此我们需要把参数设置为空气指针

2)等待一个固定时间:在仅有一个描述符准备好后IO才返回，但是不超过timeval 结构体所指定的秒数和微妙数

3）根本不等待，检查描述符后立即返回，被称为轮询，该参数必须指向一个timeval结构体，而且其中的定时器必须为0

前面两种情形的等待通常会被进程在等待期间捕获的信号中断，并从信号处理函数返回

timeout的const 限定词表示它在函数返回时不会被select修改。距离来说，如果我们指定一个10s的超时值，不过在定时器到时之前select 就返回了（结果可能时有一个或多个描述符就绪，也可能是得到了EINTR错误），那么timeval结构不会被更新成该函数返回剩余的描述。如果我们想直到这个值，可以在调用select之前取得系统时间，它返回后取得系统时间，两个值相减就是该值

中间的三个参数readset，writeset和exceptset指定我们要让内核测试读、写、和异常条件的描述符。目前支持的异常条件主要有2个：

1）某个套接字的带外数据到达

2）某个已经置为分组模式的伪终端可以从其主端读取的控制状态主信息

如何给这三个参数中的每一个参数指定一个或多个描述符值是一个设计上的问题。select使用描述符集，通常是一个整数数组，其中每个整数中的每一位对应一个描述符

	void FD_ZERO(fd_set *fdset);
	void FD_SET(int fd,fd_set *fdset);
	void FD_CTL(int fd,fd_set *fdset);
	int FD_ISSET(int fd,fd_set *fdset);

头文件<sys/select.h>中定义的FD_SETSIZE常值是数据类型fd_set中的描述符总数，其通常值是1024，不过很少有程序可以用到这么多的描述符，我们指定的maxfdp1必须要再最大描述符基础上进行加1原因是描述符都是从0开始的，但是这里的是最大个数是从1开始的所以必须要对最大描述符进行加一

select函数修改由指针readset、writeset和exceptset所指向的描述符集，因而这三个参数都是值-结果参考。调用该函数的时候，我们指定所关心的描述符的，函数返回时，结果将指示那些描述符已经就绪，该函数返回后，我们要使用FD_ISSET宏来测试fd_set数据类型中的描述符。描述符集内任何与未就绪描述符对应的返回均清0。为此，每次重新调用select函数时，我们得再次把所有描述符集内所关心的位均置1

该函数返回值表示跨所有描述符集的已经就绪的总位数。如果定时器到时间返回0，错误返回-1

######3.1描述符的就绪条件

我们一直在讨论某个描述符准备好IO或是等待其上发生一个待处理的异常条件（带外数据）。尽管可读性和可写性对于普通文件这样的描述符显而易见，然而对于select返回套接字“就绪”条件我们必须要讨论的更加明白一些

(1)满足下面四个条件套接字满足读的条件

a)套接字接收的缓冲区中数据字节数大于等于套接字接收缓冲区低水平位标记当前的大小。对于这样的套接字执行读操作不会阻塞并且返回一个大于0的值（也就是返回准备好读入的数据）。我们可以使用SO_RCVLOWAT套接字选项设置该套接字的低水位标记。对于TCP和UDP而言默认是1.

b)该连接的读半部关闭（也就是接收了FIN的TCP连接）。对这样的套接字不会阻塞并将返回一个大于0的值（也就是返回准备好读入的数据）。

c)该套接字是一个监听套接字且已经完成的连接数不是0.对于这样的套接字accept 通常不会阻塞，在15.6中可能讲accept阻塞的时序条件

d)其上有一个套接字错误待处理。对于这样的套接字读操作将不会阻塞并且直接返回-1(也就是返回一个错误),同时把errno设置成错误条件。这些待处理错误（pending error）也可以通过指定SO_ERROR套接字选项，通过getsockopt获取并且清除

（2）满足以下四个条件套接字准备好可写

a)该套接字发送缓冲区中的可用空间字节数大于等于套接字发送缓冲区的低水位标记的当前大小，并且或者该套接字已经连接，或者该套接字不需要连接。这意味着这样的套接字我们设置成为了非阻塞，写操作将不阻塞并且返回一个正值（例如由传输层接受的字节数）。我们可以使用SO_SNDLOWAT套接字选项来设置套接字的低水平标记位。对于TCP、UDP而言，这个默认值通常是2048

b)连接的写半部已经关闭对这样的套接字通常会发生SIGPIPE信号

c)使用非阻塞式connect已经建立或者connect以失败告终

d)其上有一个套接字错误待处理。对于这样的套接字的写操作将不阻塞并且返回-1(也就是返回一个错误)，同时把errno切换成错误条件，也可以通过SO_ERROR的getsockopt函数来清除

注意：当某个套接字上发生错误时候，它将由select标记又可读又可写

接收低水平位和发送低水平位标记的目的在于：允许控制程序控制select 返回可读或者可写条件之前有多少数据可读或有多大空间可写。距离来说，我们直到除非至少存在64个字节数据，否则我们的进程没有任何有效的数据可做，那么可以把接收低水平位设置为64，以防止少于64个字节的时候准备好刻度select 告诉我们

任何UDP套接字只要其发送低水平位标记小于等于发送缓冲区大小，就总是可写的因为UDP不要连接


总结：

select 返回可写可读不仅仅只有收到数据或 可以发送数据时候才会返回，还有的时候连接字半部已经关闭或者接收套接字接收半部已经关闭同样会返回，以及套接字出错依旧会返回，以及侦听套接字和connect也依旧会出发返回


FD_ZERO(fd_set *fdset);将指定的文件描述符集清空，在对文件描述符集合进行设置前，必须对其进行初始化，如果不清空，由于在系统分配内存空间后，通常并不作清空处理，所以结果是不可知的。

 FD_SET(fd_set *fdset);用于在文件描述符集合中增加一个新的文件描述符。
 
 FD_CLR(fd_set *fdset);用于在文件描述符集合中删除一个文件描述符。
 
 FD_ISSET(int fd,fd_set *fdset);用于测试指定的文件描述符是否在该集合中。

####4.str_cli修订版

早期的str_cli函数程序可能永远阻塞在fgets的调用。用select可以解决这个问题

客户的套结字上的三个条件处理如下:

1)如果对端TCP发来数据那么套接字返回可读read 返回大于0

2)如果对端套接字发送FIN那么read 返回EOF

3)如果对端发送RST，那么套接字返回-1，而且errno中有确切错误码

调用select:

我们只需要设置一个检查可读性的文件描述符集。集合首先FD\_ZERO的初始化，用FD_SET打开两个位，一位用于标准IO的文件指针，一位用于套接字sockfd。fileno函数把标准的文件描述指针转化成对应的文件描述符。select和poll函数只工作在描述符上，计算处两个描述符中的较打的以后，在调用中，写集合和异常集合指针都为空，因为我们希望调用到某个描述符就绪后为止

处理可读套接字：

套接字可读就用readline 读取，fputs输出

处理可读输入:

如果标准输入可读就先用fgets读入一行，再用writen写入到套接字之中。

本版本中相同的四个IO函数 fgets fputs writen readn
新的版本使用select 驱动旧的版本采用fgets 驱动

实例代码：
	#include "unp.h"

	void str_cli(FILE* fp,int sockfd)
	{
		int maxfdpl;

		fd_set rset;

		char sendline[MAXLINE],recvline[MAXLINE];

		FD_ZERO(&rset);

		for(;;){
			FD_SET(fileno(fp),&rset);
			FD_SET(sockfd,&rset);
			maxfdpl = max(fileno(fp),sockfd)+1;

			select(maxfdpl,&rset,NULL,NULL,NULL,NULL);

			if(FD_ISSET(sockfd,&rset)){
				if(readline(sockfd,recvline,MAXLINE) == 0)
				{
					err_quit("str_cli:server terminated");
				}
				fputs(recvline,stdout);
			}

			if(FD_ISSET(fileno(fp),&rset)){
				if(fgets(sendline,MAXLINE,fp) == NULL)
				{
					return;
				}
				writen(sockfd,sendline,strlen(sendline));
			}
		}
	}

####6.5批量输入

str\_cli函数依然不正确。首先让我们回到最初的版本，即图5-5,它以停-等的方式工作，这时交互式使用是最合适的：发送一行 文本给服务器，然后等待应答。这段时间是往返时间(round-trip time RTT)加上服务器处理的时间（相对于回射服务器而言处理时间几乎是0）。如果知道了客户与服务器之间的RTT，我们便可以估计出回射固定的行需要多久.

ping程序是测试RTT	一个十分简单的方法

如果我们把客户与服务器之间的网络作为全双工管道来考虑，请求是从客户向服务器发送的，那么图6-10展示了这样的停-等方式

	时刻0:
		客户:->
		服务:
	时刻1:
		客户:	->
		服务:
	时刻2:
		客户:		->
		服务:
	时刻3:
		客户:			<->
		服务:
	时刻4:
		客户:
		服务:			<-
	时刻5:
		客户:
		服务:		<-
	时刻6:
		客户:
		服务:	<-
	时刻7:
		客户:
		服务:<-

图中->代表请求 <-代表应答

客户在时刻0开始出发请求，假设我们RTT为8个时间单位。其应答在时刻4发出并在时刻7接收。我们还是假设在没有服务器处理时间而且请求应答时间大小相同。图中只是展示了客户与服务器之间的数据分组，而忽略了同样穿越网络的TCP确认。

既然一个分组从管道的一段发出到达管道的另一端存在延迟，而管道是全双工的，就本例子而言。我们使用了管道容量的1/8.这一停等的方式是十分合适的。然而由于我们客户是从标准输入读并往标准输出写。unix的shell环境下重定向标准输入和标准输出是轻而易举之事，我们可以很容易的以批量方式运行客户。我们把标准输入和输出重定向到文件来运行客户程序时，发现输出文件总是小于输入文件（而对于服务器而言他们理应相等）

为了搞清楚到底发生什么，我们应该意识到在批量方式下，客户能够以网络可以接受的最快的速度持续发送请求，服务器以相同的速度处理它们并发回应答，这时就会导致管道被塞满

	客户:->->->->
	服务:<-<-<-<-

这里我们假设发出第一个请求后，立即发送下一个，紧接着再下一个，假设我们客户能够可以接受它们的最快速度持续发送请求，并且网络可提供它们最快速度的应答。

为了搞清楚图6-9的str\_cli函数存在的问题，我们假设输入文件还有九行。最后一行在时刻8发出，如图6-11所示。写完这个请求后，我们并不能立即关闭链接，因为管道中还有其他请求和应答。问题起因在于我们对标准输入中的EOF的处理:str\_cli函数就此返回到main函数，而main函数随后则终止，然而在批量的模式下，标准输入中的EOF并不意味着我们同事也完成了套接字的读入，可能荣然有请求在去往服务器的路上，或者仍然有请求在返回客户端的路上。

我们需要一种半关闭TCP连接的方法。也就是说我们想给服务器发送一个FIN,并告诉我们已经完成了数据发送，但是仍然保持套接字描述符打开可以方便读取，这可以由shutdown函数来完成。

一般地说，为了提升性能而引入缓冲机制增加了网络应用程序的复杂性，图6-9就深受复杂之苦。考虑来自多个标准输入和文本输入可用的情况。select将第20行代码用fgets读取输入，这又转而使已经可用的文本输入行被读取到stdio可用的缓冲区中。然而fgets之返回其中一行，其余仍在stdio缓冲区中，第22行代码把fgets返回的单个输入行写给服务器，随后select再次被调用以等待新的工作，而不管stdio缓冲区中还有额外的输入行代销费。究器原因在于select不知道使用了stdio缓冲区，只是从read系统调用的角度指出是否有数据可读，而不是匆匆fgets之类的调用角度考虑，基于上述原因，混合使用stdio和select被认为是错误的需要十分小心

select也不能看见readline缓冲区的数据

####6.6 shutdown函数

终止 网络通常调用close函数不过close 函数有两个限制

1.close把描述符计数器减一，仅在计数器为0的时候才关闭套接字。

2.close终止读和写两个方向的数据传输。既然TCP是全双工的有时我们需要告知对端我们已经完成了数据发送，即使对端仍有数据要发送给我们。（即使数据没发送完也会关闭套接字）

我们可以调用shutdown关闭一般连接

	#include <sys/socket.h>
	int shutdown(int sockfd,int	howto);

howto参数:

SHUT_RD

关闭读这一半，套接字不可能再有数据接收，而且套接字接收缓冲区中现有的数据全部丢弃。进程不能再对这样的套接字进行任何读函数。对一个TCP套接字这样调用shutdown函数之后，由该套接字接收的任何来自对端的数据都被确认然后丢弃

SHUT_WR

关闭连接写这一半--对于TCP套接字，称为半关闭。当能留在套接字发送缓冲区中的数据将被发送掉，然后跟TCP的正常连接终止序列，不管计数器是否为0，这样写半步关闭照样执行，进程不能对套接字进行任何写操作

SHUT_RDWR

连接读半部和写半部都关闭，与调用两次shutdown等效：第一次调用SHUT_RD，第二次调用SHUT_WR

将汇总调用shutdown或者close的各种可能。close取决于SO_LINGER套接字选项的值。


####str_cli再修订

给出str_cli函数的改进版，他是用了select和shutdown，其中只要服务器关闭它一端的链接就会通知我，后者允许我们正确的处理批量输入。这个版本废弃了以文本为中心的代码，改而针对缓冲区操作，消除了6.5的复杂性。

	#include "unp.h"
	
	void str_cli(FILE* fp,int sockfd)
	{
		fd_Set rset;
	
		FD_ZERO(&rset);
	
		int stdineof = 0;
	
		int maxnum;

		char buf[MAXLINE];
	
		for(;;)
		{
			if(stdineof == 0)
			{
				FD_SET(fileno(fp),&rset);
			}
	
			FD_SET(sockfd,&rset);
	
			maxnum = max(fileno(fp),sockfd)+1;
			
			select(maxnum,&rset,NULL,NULL,NULL);

			if(FD_ISSET(sockfd,&rset))
			{
				if((n = read(sockfd,buf,MAXLINE)) == 0)
				{
					if(stdinteof == 1)
						return;
					else
						err_quit("server terminated");
						
				}
				write(fileno(stdout),buf,n);
			}	

			if(FD_ISSET(fileno(fp),&rset))
			{
				if((n = read(fileno(fp),buf,MAXLINE)) == 0)
				{
					stdineof = 1;
					shutdown(sockfd,SHUT_WR);
					FD_CLR(fileno(fp),&rset);
					continue;
				}
				write(sockfd,buf,n);
			}
		}
	}

stdineof是一个初始化0的标识符。只要是0 我们每次都需要select描述符的可读性

如果我们sockfd遇到了EOF，如果标准输入上没有EOF则代表服务器终止

如果我们在标准输入上遇到了EOF，那么我们应该首先，将stdinteof标志位定位1，然后从select中删除标准输入的监听，然后半关闭套接字（写这一段）然后continue继续监控服务器读（防止数据读完），最后终止进程

####8.TCP服务器回射程序（修订）

我们打算使用select来处理任意个客户进程，而不是每个客户派生一个子进程。在给出具体代码前,让我们先查看用以跟踪的客户的数据结构。

	单个监听描述符，服务器维护一个描述符集。假设服务器是前台启动，那么描述符0、1和2将分别被设置为标准输入和标准输出和标准错误输出。可见监听套接字的第一个可用描述符是3。

当一个客户端和服务端建立连接的时候，监听描述符为可读，我们的服务器调用了accept，在本例子的假设下，由accept返回新的已经连接的描述符是4。

我们的服务器必须在其client数组中记住每个新的已连接的描述符，并把它加到描述符集中去。展示了更新后的数据结构。

我们接着假设一个客户终止他的连接，select返回套接字，read读取套接字这时候返回0，我们把client数组更新为-1，把client[0]的值设为-1，把描述符4的位置设置为0。

总之当有客户到达时，我们在client数组中的第一个可用项（值为-1的第一项）中记录其中已经连接的套接字描述符。我们必须把已连接的描述符加入到读描述符集中。变量maxi是client数组当前使用项的最大下标，而变量maxfd加1后是select的第一个参数的当前值。对于本服务器所能处理的最大客户数目限制是FD_SETSIZE和内核允许本进程打开的最大描述符中的较小者。

	#include "unp.h"
	
	int main(int argc,char **argv)
	{
		int i,maxi,maxfd,listenfd,connfd,sockfd;
	
		int nready,client[FD_SETSIZE];
	
		ssize_t n;
	
		fd_set rset,allset;
	
		char buf[MAXLINE];
	
		socklen_t client;
	
		struct sockaddr_in cliaddr,servaddr;
	
		listenfd = socket(AF_INET,SOCK_STREAM,0);
	
		servaddr.sin_faimly = AF_INET;
	
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	
		servaddr.sin_port = htons(SERV_PORT);
	
		bind(listenfd,(struct sockaddr *)servaddr,sizeof(servaddr));
	
		listen(listenfd,LISTENQ);
	
		maxfd = listenfd;//初始化
	
		maxi = -1;
	
		for(i = 0;i<FD_SETSIZE;i++)
		{
			client[i] = -1;
		}
	
		FD_ZERO(&allset);
	
		FD_SET(listenfd,&allset);
	
		for(;;)
		{
			rset = allset;
	
			nready = select(maxfd+1,&rset,NULL,NULL,NULL);
	
			if(FD_ISSET(listenfd,&rset))
			{
				clilen = sizeof(cliaddr);
				connfd = accept(listenfd,(struct sockaddr *)cliaddr,&clilen);
	
				for(i=0;i<FD_SETSIZE;i++)
				{
					if(client[i] < 0){
						client[i] = connfd;
						break;
					}
				}
	
				if(i == FD_sETSIZE)
					err_quit("too many clients");
	
				FD_SET(connfd,&allset);
	
				if(connfd>maxfd)
					maxfd = connfd;
	
				if(i>maxi)
					maxi = i;
	
				if(--nready <= 0)
					continue;
			}
	
			for(i = 0; i<=maxi;i++)
			{
				if((sockfd = client[i]) < 0)
					continue;
	
				if(FD_ISSET(sockfd,&rset))
				{
					if((n = read(sockfd,buf,MAXLINE)) == 0){
						close(sockfd);
						FD_CLR(sockfd,&allset);
						client[i] = -1;
					}else{
						writen(sockfd,buf,n);
					}
	
					if(--nready <= 0)
						break;
				}
			}
		}
	}

######1.
创建监听套接字并为select初始化

创建监听套接字在与早期版本一样：socket、bind和listen。

select的唯一描述符是监听描述符之一前提初始化我们的数据结构

######2.阻塞于select：

select等待某个事情发生，或是新客户的建立，或是FIN，或是数据到达，或是RST

######3.accept新的连接

如果监听套接字可读，那么就是已经建立起一个新的连接。我们调用accept并相应地更新数据结构，使用client数组中的第一个未用项记这个以连接的描述符。就绪描述符数目-1，若值变为0，就可以避免进入下一个for循环，这样我们就可以使用select避免未就绪的描述符

######4.检查现有连接

对于每个现有的客户连接，我们要测试其描述符是否在select返回的描述符集中，如果是该客户读入一行文本并且回射给他。如果该客户关闭了连接read 返回0，我们应该更新数据结构。我们从不减少maxi的值，不过每当有客户关闭链接时，我们可以检测是否存在这样的可能性。

这种处理方式避免了每一个客户套接字创建一个进程对资源的占用

拒绝服务型攻击:

不幸的是：我们刚才的程序性是存在问题的。考虑一下如果一个恶意的客户连接到服务器，发送一个字节后进入睡眠，服务端从客户端read后将会阻塞在此，以等待来自该客户端的其余数据，于是不会进入下一个for循环，不会再处理其他客户套接字了。

于是我们在这里有了一个基本概念：当一个服务器在处理多个客户的时候，决不能阻塞与单个客户相关的某个函数，解决方案如下：

1)使用非阻塞IO

2）让每个客户由单独的控制现场处理

3）设置一个IO超时时间

####6.9 pselect函数 

pselect是posix发明的，如今由许多unix变种支持他

	#include <sys/select.h>
	#include <signal.h>
	#include <time.h>

	int pselect(int maxfdpl,fd_set *readset,fd_set *write_set,fd_set *except_set,const struct to,es[ec *timeout,const	sig_set *sigmask);

pselect相对于通常的select有两个变化。

（1）pselect使用timespec结构，而不是使用timeval结构体。timespec结构是posix的又一个发明。

	struct timespec{
		time_t tv_sec;
		long tv_nsec;
	}
区别在第二个成员，tv_nsec 指定纳秒数,tv_usec指定的是微妙数

 (2)pselect第六个参数：一个指向信号的掩码指针。该参数允许程序先禁止递交某些信号，再测试这些当前被禁止的信号的	信号处理函数设置的全局变量然后调用pselect，告诉它重新设置信号掩码

select也是一种慢函数

select后 也需要加 errno为ENTER的判断

	if(intr_flag)
		handle_intr();
	
	if((nready = select()) < 0)
	{
		if(errno = EINTR){
			if(intr_flag)
				handle_intr();
		}
	}

####6.10 poll 函数

	#include <poll.h>

	int poll(struct pollfd *fdarray,unsigned long nfds,int  timeout);

第一个参数是指向一个结构数组的第一个元素指针。每个数组元素都是一个pollfd结构，用于指定测试某个给定fd的条件。

	struct pollfd{
		int fd,
		short events;
		short revents;
	}	

要测试的条件由events成员指定,函数在相应的revents成员中返回描述符的状态。（每个描述符都有两个变量，一个为调用值，另一个为返回结果，从而避免了值结果参数。回想select的中间三个参数都是值结果参数。）这两个成员中的每一个	都由指定的某个特定条件的一位或多位构成。

处理输入的三个常值：

	POLLIN 普通优先带数据可读
	
	POLLRDNORM	普通数据可读
	
	POLLRDBAND	优先级带数据可读
	
	POLLPRI	高优先级数据可读

处理输出的三个常值：

	POLLOUT 普通数据可写

	POLLWRNORM 普通数据可写

	POLLWRBAND 优先级带数据可写

错误处理的三个常值

	POLLERR	发生错误
	POLLHUP	发生挂起
	POLLNVAL	描述符不是一个可打开的文件

其中错误处理的三个常值不能在events 而是在revents中处理。

poll 识别三类数据：

普通（normal） 优先带级（priority band）和高优先级（high priority）

就TCP和UDP而言，一下条件引起poll返回特定的revent，不行的是POSIX在其poll的定义中留下了许多空洞

	所有正规的TCP和UDP数据都被认为是普通数据

	TCP的带外数据被认为是优先级带数据

	TCP连接的读半部关闭的时候，也被认为是普通数据，随后的读操作返回0

	TCP连接存在错误即可认为是普通数据，也可以认为是(POLLERR)。无论哪种情况随后的读操作都返回-1。并把errno设置为合适的值，这可用于设置RST或发生超时条件。

	在监听套接字上有新的连接既可以认为是普通数据，也可以视为优先级数据，大多数都视之为普通数据

	非阻塞使connect完成被认为是相应的套接字可写。

timeout参数指定poll等待多长时间，他是一个指定对应毫秒数的正值

	INFTIM 永远等待

	0	立即返回，不阻塞进程

	>0  等待指定书目的毫秒数

当发生错误的时候poll返回-1，如果定时器到时之前没有任何描述符就绪返回0，否则返回就绪个数，即revents为非0的描述符个数

如果我们不在关心某个描述符，可以把pollfd的fd成员设置为负值，poll函数将忽略pollfd的events成员。返回时将revents置位0。

我们就FD_SETSIZE以及每个描述符集中最大描述符数目比每个进程中最大描述符书目展开讨论。有了poll就可以不再关心这个问题，因为分配一个pollfd结构的数组并把数组中元素的数目通知内核成了调用者的责任。内核不再需要知道类似fd_set的固定大小的数据类型。

####6.11 TCP回射服务程序

	#include "unp.h"
	
	int main(int argc,char **argv)
	{
		int i,maxi,maxfd,listenfd,connfd,sockfd;
	
		int nready;
	
		ssize_t n;
	
		fd_set rset,allset;
	
		char buf[MAXLINE];

		struct pollfd client[OPEN_MAX];
	
		socklen_t clilen;
	
		struct sockaddr_in cliaddr,servaddr;
	
		listenfd = socket(AF_INET,SOCK_STREAM,0);
	
		servaddr.sin_faimly = AF_INET;
	
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	
		servaddr.sin_port = htons(SERV_PORT);
	
		bind(listenfd,(struct sockaddr *)servaddr,sizeof(servaddr));
	
		listen(listenfd,LISTENQ);
	
		maxfd = listenfd;//初始化
	
		maxi = 0;
	

	
		client[0].fd = listenfd;
		client[0].events = POLLRDNORM;

		for(i = 1;i<OPEN_MAX;i++)
		{
			client[i].fd = -1;
		}
	
		for(;;)
		{
			nready = poll(client,maxi +1,INFTIM);

			if(client[0].revents & POLLRDNORM){
				clien = sizeof(cliaddr);

				connfd = accept(listenfd,(struct cliaddr *)cliaddr,&clilen);

				for(i = 1;i<OPEN_MAX;i++)
				{
					if(client[i].fd < 0)
					{
						client[i].fd = connfd;
						break;
					}
				}

				if( i == OPENMAX)
				{
					err_quit("too many clients");
				}

				client[i].events = POLLRDNORM;

				if(i > maxi)
					maxi = i;

				if(--nready <= 0)
					continue;
			}

			for(i =1;i<=maxi;i++)
			{
				if((sockfd = client[i].fd) < 0)
					continue;

				if(client[i].revents & (POLLRDNORM | POLLERR))
				{
					if((n = read(sockfd,buf,MAXLINE)) < 0)
					{
						if(errno = ECONNRESET){
							close(sockfd);
							client[i].fd = -1;
						}else{
							err_sys("read error");
						}
					}else if(n == 0){
						close(sockfd);
						client[i].fd = -1;
					}else{
						writen(sockfd,buf,n);
					}
				}
			}
		}
	}
