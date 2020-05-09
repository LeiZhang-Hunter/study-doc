#第十七章ioctl操作

####17.1概述

ioctl函数传统上一直作为那些不合适归入其他精细定义类别的特性的系统接口。POSIX致力于摆脱标准化过程中的特定功能的ioctl接口。然而posix为我们创造了12个新的函数:tcggetattr用于获取终端属性，tcflush用于冲刷待处理的输入或者输出等等。类似的posix替换了一个用于网络的ioctl请求，新的sockatmark函数取代SIOCATMARK ioctl。尽管如此，为于网络编程相关并且依赖于实现的特性保留的ioctl荣然不在少数，他们用于获取接口信息、访问路由表、让问APR高速缓存等等。

网络程序经常在程序启动执行后使用ioctl获取所在主机全部网络接口的信息，包括接口地址、是否支持广播、是否支持多播，等等 我们将开发用于返回这些接口信息的函数，在本章提供一个使用ioctl的实现，第十八章提供一个使用路由套接字的实现。

####17.2 ioctl函数

	#include <unistd.h>

	int ioctl(int fd,int request,.../* void *arg*/);

其中第三个参数总是一个指针，指针的类型依赖request参数

我们把网络相关的请求划分为6类

套接字操作

文件操作

接口操作

ARP告诉缓存操作

路由表操作

流系统

我们知道ioctl的一些操作和fcntl的操作重叠，而且某些操作可以使用不止ioctl这一个函数操作

ioctl request参数以及arg地址的数据类型


	类别       request           说明               数据类型
	套接字    SIOCATMARK     是否位于带外标记         int
	         SIOCSPGRP      设置套接字的进程id或进程组id  int
	         SIOCGPGRP      获取套接字的进程id或进程组id  int
	
	文件      FIONBIO        设置清除非阻塞的IO标志     int
			 FIOASYNC       设置清除信号驱动异步IO标志  int
	         FIONREAD       设置接收缓冲区的字节数      int
			 FIOSEROWN      设置文件的进程ID或进程组ID  int
			 FIOGETOWN      获取文件的进程ID或进程组ID  int
	
	接口     SIOCGIFCONF     获取所有接口的列表	  struct ifconf
			SIOCSIFADDR		设置接口地址			  struct ifreq
            SIOCGTFADDR      获取接口地址          struct ifreq
		 	SIOCSIFFLAGS	 设置接口标志			  struct ifreq
			SIOCGIFFLAGS	 获取接口标志			  struct ifreq
			SIOCSTFDSTADDR   设置点到点标志		  struct ifreq
			SIOCGTFDSTADDR   获取点到点标志         struct ifreq
   			SIOCGIFBRDADDR    获取广播地址		  struct ifreq
			SIOCSIFBRDADDR    设置广播地址		  struct  ifreq
			SIOCGIFNETMASK    获取子网掩码          struct ifreq
			SIOCSIFNETMASK    设置子网掩码		  struct   ifreq
			SIOCGIFMETRIC     获取接口的测度		  struct ifreq
			SIOCSIFMETRIC	   设置接口的测度       struct  ifreq
			SIOCGIFMTU			获取接口的MTU		  struct ifreq
			.........
	ARP		SIOCSARP		  创建修改ARP项表		  struct  arpreq
			SIOCGARP		  获取ARP项表		  struct  qrpreq
		    SIOCDARP		  删除ARP项表          struct qrpreq
	路由		SIOCADDRT		  增加路径			  struct  rtentry
			SIOCDELRT		  删除路径			  struct rtentry
	流

####17.3套接字操作

SIOCATMARK

如果本套接字的读指针当前位于带外标记，那就由第三个参数指向的整数返回一个非0的值，否则返回一个0的值，我们将在第24章讲解带外数据。

查看套接字是否外带数据的标志位

	#include "common.h"
	#include <unistd.h>
	
	int main()
	{
		int flag;
		int fd = socket(AF_INET,SOCK_STREAM,0);
		ioctl(fd,SIOCATMARK,&flag);
		printf("result:%d", flag);
	}

SIOCGPGRP

通过由第三个参数指向的整数返回本套接字的进程id或者进程组的id，该ID指定针对本套接字的SIGIO或者SIGURP信号的接收进程。本操作和fcntl函数的F_GETOWN命令等效

SIOCSPGRP

本套接字的进程ID或进程组ID设置成由第三个参数指向的整数，该ID指定针对本套接字的SIGIO或者SIGURP信号的接收进程。本请求和fcntl的F_SETOWN命令等效。
			
####17.4 文件操作

下一组请求以FIO打头。

FIONBIO  

根据ioctl的第三个参数指向一个0值或者是一个非0值，清除或者设置本套接字的非阻塞IO标志。本请求和O_NONBLOCK文件标志等效，还可以通过fcntl的F_SETFL命令清除或设置这个标志。

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

FIOASYNC 

根据ioctl的第三个参数指向一个0值或非0值，可清除或设置针对这个套接字的信号驱动IO标志。他决定是否收取针对本套接字的异步IO信号(SIGIO).本请求和O_ASYNC文件标志等效，而且还可以通过fcntl的F_SETFL命令清除或者设置该标志。

FIONREAD

通过由ioctl的第三个参数指向的整数返回当前在套接字接收缓冲区中的字节数。

FIOSETOWN

对于套接字和SIOCSPGRP等效 

FIOGETOWN

和套接字选项SIOCGPGRP等效

####17.5接口配置

处理网络接口许多程序沿用的初始步骤之一就是从内核获取配置在系统中的所有接口。本任务由SIOCGIFCONF请求完成，它使用ifconf结构，ifconf又使用ifreq结构

结构体内容展示

	struct ifconf{
		lint ifc_len;
		union{
			caddr_t ifcu_buf;
			struct ifreq *ifcu_req;
		}
	};

	#define ifc_buf	ifc_ifcu.ifcu_buf
	#define ifc_req ifc_ifcu.ifcu_req

	#define IFNAMSIZ 16

	struct ifreq{
		char ifr_name[IFNAMSIZ];
		union{
			struct sockaddr ifru_addr;
			struct sockaddr ifru_dstaddr;
			struct sockaddr ifru_broadaddr;
			short ifru_flags;
			int ifru_metric;
			caddr_t ifru_data;
		}ifr_ifru;
	}

	#define ifr_addr ifr_ifru.ifru_addr
	#define ifr_dstaddr ifr_ifru.ifru_dstaddr
	#define ifr_broadaddr ifr_ifru.ifru_broadaddr
	#define ifr_flags ifr_ifru.ifru_flags
	#define ifr_metric ifr_ifru.ifru_metric
	#define ifr_data ifr_ifru.ifru_data

指向某个ifreq结构的指针组用也在图17-1所展示的接口类其余ioctl请求的一个参数。注意ifreq含有一个联合体，而很多define 隐藏了众多字段属于联合的事实。对于这个联合某个成员的所有引用都使用如此定义的名字。注意有一些系统往这个ifr_ifru联合中添加了许多依赖于实现的成员。

####17.6 get\_ifi\_info函数

代码实例:https://mp.csdn.net/postedit/86415720

既然有十分多的程序需要知道系统中所有的接口，我们于是开发了一个get_ifi_info函数，它返回了一个链表结构，其中每个结构对应当前处于up的接口。我们在本节，使用SIOCGIFCONF ioctl实现这个函数，第十八章将开发一个使用路由套接字的版本。

我们首先在一个名字是unpifi.h的新文件中定义ifi_info函数，它返回一个结构链表，其中每个结构体的if_next成员指向下一个结构体。我们在本节中返回了典型的应用程序可能关注的信息：接口名字、接口索引、MTU、硬件地址、接口标志（用来判断接口是否支持广播和多播，或是一个点到点的接口）、接口地址、广播地址、点到点链路的目的地址。用于存放ifi_info结构和其中所含套接字地址结构的内存空间都是动态获取的。我们还分配了一个free_ifi_info函数动态分配内存空间。

1.创建一个用于ioctl的套接字（tcp或者udp都可以）

2.在一个循环中发出SIOCGIFCONF请求

SIOCGIFCONF请求存在一个严重问题，当缓冲区大小不足的时候，一些实现不返回错误，而是阶段结果并返回成功，要解决这一问题的解决办法是发出请求，记录下返回的长度，用更加大的缓冲区去请求，比较返回的长度和记下的长度，只有两个长度相同，才表示我们的缓冲区足够大了。

动态分配一个缓冲区，一开始为100个ifreq结构的空间，在lastlen中记录最近一次SIOCGIFCONF请求的返回长度，其初始值是0

如果ioctl调用返回一个EINVAL错误，并且返回的ioctl调用未曾发出过，我们调用的缓冲区还是不够大，那么我们就继续循环。

如果ioctl调用成功，并且返回的长度等于lastlen，那么与上次ioctl的调用返回的长度没有变化，于是break跳出循环，因为我们已经得出了接口信息。

每次循环把缓冲区的长度扩大10个ifreq字节

3.初始化链表的指针

既然要返回某个ifi_info结构链表之头结构的一个指针，我们于是使用两个变量ifhead和ifipnext链表构造过程中保存指针。

4.步入下一个套接字的地址结构

在遍历所有ifreq结构过程中，ifr指向每个结构体，我们随后增长ptr以指向下一个结构体。这里我们必须及处理为套接字地址结构提供长度字段较为新的地址，又要不提供这个长度的老操作系统

5.处理AF_LINK

如果系统支持在SIOCGIFCONF中返回AF_LINK地址族结构的sockaddr结构，我们就从中复制接口索引和硬件地址信息。

忽略所有不是调用者期望的地址簇地址。

6.处理别名地址

我们必须检测当前接口可能存在的任何别名地址。注意用于别名地址的接口名字中又一个冒号。为了处理这两种情况，我们把最近处理过的接口名字存入lastname，并且在与当前接口名字比较时候，若有冒号则比较到冒号。不论是否又冒号，如果比较结果相同就忽略当前接口。

7.获取接口标志

我们发出一个ioctl的SIOCGIFFLAGS请求以获取接口标志。ioctl的第三个参数是指向某个ifreq结构的一个指针，这个结构中必须包含要获取标志的接口的名字。这个结构是在调用ioctl之前从某个ifreq结构复制成的，因为如果不这么做，ioctl调用将覆写当前ifereq结构中已经有的ip地址，因为接口标志和ip地址在ifreq结构中是同一个联合的不同成员。如果当前接口不处于工作状态就忽略它。

8.分配并且初始化ifi_info结构

至此我们指导将向当前调用者返回当前接口。我们动态分配一个ifi_info结构，并把它加到结构中链表的尾部，而且既然calloc已经把所在分配区域全部初始化为0，我们指导ifi_next已被初始化为空指针。我们复制保存的几口索引和硬件地址长度，若该长度部位0则同时复制保存硬件地址。

把由最初的SIOCGIFCONF请求返回的IP地址复制到我们正在构造的结构中

如果当前接口支持广播，就用ioctl的SIOCGIFBRDADDR请求取得广播地址。我们动态分配一个套接字地址结构存放该地址，并把它放到正在构造的ifiinfo结构体中

####17.7接口操作

这些请求接受返回一个ifreq结构中的信息，而这个结构的地址则作为ioctl调用的第三个参数指定，接口总是以其名字标识，在ifreq结构的ifr_name成员中指定

这些请i去中由许多使用套接字地址结构在应用进程和内核之间指定或返回具体接口的ip地址或者是地址掩码。对于IPV4，这个地址或掩码存放在一个网际套接字的sin_addr成员中；对于IPV6,它是一个ipv6套接字的sin6_addr成员。

SIOCGIFADDR   ifr_addr成员中返回单播地址

SIOCSIFADDR    ifr_addr成员设置接口地址。

SIOCGIFFLAGS    ifr_flags成员中返回接口标志，这些标志返回格式为IFF_XX 在<net/if.h>中定义

SIOCSIFFLAGS    在ifr_flags成员设置接口标志

SIOCGIFDSTADDR   在ifr_dstaddr成员中返回点到点的地址

SIOCSIFDSTADDR    在ifr_dstaddr成员中设置点到点的地址

SIOCGIFBRDADDR    在ifr_broadaddr成员中返回广播地址。应用进程必须首先获取接口标志，然后发出正确请求：对于广播接口为SIOCGIFBRDADDR，对于单播地址为SIOCGIFDSTADDR

SIOCSIFBRDADDR  在ifr_broadaddr成员中设置广播地址

SIOCGIFNETMASK   在ifr_addr成员中返回子网掩码

SIOCSIFNETMASK    在ifr_addr成员中设置子网掩码

SIOCGIFMETRIC   ifr_metric返回接口的测度

SIOCSIFMETRIC   用ifr_metric设置接口的路由测度


17.8  高速缓冲操作

ARP高速缓存也通过ioctl函数操作。使用路由域套接字的系统往往改用由路由套接字访问ARP告诉缓存。这些请求使用一个如图17=12的arpreq结构，定义在同文件<net/if_arp.h>中。

	struct arpreq{
		struct sockaddr arp_pa;
		struct sockaddr arp_ha;
		int arp_flags;
	}

	#define ATF_INUSE 0X01
	#define ATF_COM 0x02
	#define ATF_PERM 0x04
	#define ATF_PUBL 0x08

例子：输出主机的硬件地址

1.通过get_ifi_info获取所有ip地址，然后再一个循环中遍历每个地址。

2.输出ip地址

3.发出ioctl请求并且检查错误

4.输出硬件地址

	#include "unpifi.h"
	#include <net/if_arp.h>
	
	int main(int argc,char** argv)
	{
		int sockfd;
		struct ifi_info *ifi;
		unsigned char *ptr;
		struct arpreq arpreq;
		struct sockaddr_in *sin;
	
		sockfd = socket(AF_INET,SOCK_DGRAM,0);
	
		for(ifi = get_ifi_info(AF_INET,0);ifi!=NULL;ifi = ifi->next)
		{
			sin = (struct sockaddr_in*)&arpeq_arp_pa;
			memcpy(sin,ifi->ifi_addr,sizeof(struct sockaddr_in));

			if(ioctl(sockfd,STOCGARP,&arpreq) < 0){
				err_ret("ioctl SIOCGARP");
				continue;
			}

			ptr = &arpreq.arp_ha.sa_data[0];
			printf("%x:%x:%x:%x:%x:%x\n",*ptr,*(ptr+1),*(ptr+2),*(ptr+3),*(ptr+4),*(ptr+5));
		}
		exit(0);
	}

####17.9路由表操作

有些系统提供了用于操作路由表的ioctl请求。这2个请求要求ioctl的第三个参数是指向某个rtentry结构的一个指针，该结构定义再<net/route.h>头文件中。这些请求通常由route程序发出。只有超级用户才能发出这些请求。这些请求改由路由套接字而不是ioctl执行。

SIOCADDRT往路由表增加一个表项。

SIOCDELRT从路由表中删除一个表项。

####17.10 小结

用于网络编程的ioctl分为6类

1.套接字操作

2.文件操作

3.接口操作

4.ARP表操作

5.路由表操作 

6.流系统操作