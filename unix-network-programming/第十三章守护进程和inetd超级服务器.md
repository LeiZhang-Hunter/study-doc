#第十三章守护进程和inetd超级服务器

####13.1概述

守护进程是在后台运行的而且不受任何控制终端关联的进程，unix大约有20~50个守护进程 执行不同的管理任务。

守护进程没有控制终端通常源于他们由系统初始化脚本启动。然而守护进程也可能从某个终端由用户在shell提示符下键入命令行启动，这样守护进程必须亲自脱离与控制终端的关联，从而避免了与作业控制、终端会话管理、终端信号等发生任何不期望的交互，也可以避免在后台运行守护进程非预期的终端。

守护进程有多种启动办法。

1）在系统启动阶段，许多守护进程由系统初始化脚本启动。这些脚本通常位于/etc目录或以/etc/rc开头的某个目录中，他们的具体位置和内容却是现实相关的。由这些脚本启动的守护进程一开始拥有超级用户特权。

有若干个网络服务通常从这些脚本启动:inetd服务器，邮件服务器，web服务器。

2）许多网络服务由inetd 超级服务器启动。inetd自身由上一条中的某个脚本启动。inetd监听网络请求，每当有一个请求到达的时候，启动相应的实际服务器。

3）cron 守护进程按照规则定期执行一些程序，它同样作为守护进程来运行

4）at 命令通常用于指定某一个时刻执行程序，而由他执行启动的程序同样会作为守护进程来执行，通常由cron来启动执行它们，所以他们同样作为守护进程来运行。

5）守护进程可以从用户终端或在前台或在后台执行。这么做往往是为了测试守护程序或重启因某种原因而终止了守护进程

因为守护进程没有终端控制，所以有事情发生他们需要输出消息，而这种消息即是普通的通告消息或者是由管理员发出的紧急通知。syslog就是输出这个消息的标准方法。

####13.2 syslogd 守护进程

Unix系统中syslogd守护进程通常由某个系统初始化脚本启动，syslog的启动分为以下几个步骤:

1)读取配置文件。通常为/etc/syslog.conf的配置文件指定守护进程可能收取的各种守护进程日志消息。这些消息可能被添加到一个文件(/dev/console文件是一个特例，把它的消息写到控制台上)，或被写到指定用户的登陆窗口，或者被写道指定用户的登陆窗口，或者发到另一个主机的syslogd 上。

2)创建一个unix域数据报套接字，把它捆绑路径名为/var/run/log（也可能是devlog）

3）创建一个udp套接字，绑定端口是514

4)打开路径名为/dev/klog。来自内核任何出错消息看着像是这个设备的输入

syslogd 是一个死循环，调用select监控234步骤，其中之一如果变为可读就读入日志消息。这样我们可以通过syslogd 绑定的路径达到发送日志消息目的，更加简单的是我们呢可以通过udp套接字，通过往回环地址和514端口发送消息，达到写日志的目的。

注意centos6以后不再有syslog.conf了，同意改为了rsyslog.conf

####13.3syslog函数

既然守护进程没有终端，他们就不能把消息fprintf到stderr上，从守护进程中登记消息的常用技巧是调用syslog 函数

	#include <syslog.h>

	void syslog(int priority,const char* message,...);

priority参数是级别level和设施facility两者的组合，分别如图13-1和图13-2，message类似printf 的const char* 参数，不过增加了%m的规范，它将当前errno替换成出错消息，message参数的末尾可以出现一个换行符不过并非必须。

日志消息level 从0到7

	level              值            说明
	
	LOG_EMERG          0            系统不可用
	LOG_ALERT  		   1			必须立即采取行动
	LOG_CRIT		   2			临界条件
	LOG_ERR			   3			出错条件
	LOG_WARING		   4			警告条件
	LOG_NOTICE		   5			正常然而重要的条件
	LOG_INFO		   6			通告消息
	LOG_DEBUG		   7			调试消息	

日志消息还包含一个用于标识信息发送进程类型的facility。列出了facility的各种值，如果未指定默认为LOG_USER

	LOG_AUTH	安全授权消息
	LOG_AUTHPRIV	安全授权消息（私用）
	LOG_CRON	cron守护进程
	LOG_DAEMON	系统守护进程
	LOG_FTP		FTP守护进程
	LOG_KERN	内核消息
	LOG_LOCAL0	本地使用
	LOG_LOCAL1	本地使用
	LOG_LOCAL2	本地使用
	LOG_LOCAL3	本地使用
	LOG_LOCAL4	本地使用
	LOG_LOCAL5	本地使用
	LOG_LOCAL6	本地使用	
	LOG_LOCAL7	本地使用
	LOG_LPR		行式打印机系统
	LOG_MAIL	邮件系统
	LOG_NEWS	网络新闻系统
	LOG_SYSLOG	syslogd内部产生的消息
	LOG_USER	任意用户级别消息
	LOG_UUCP	UUCP系统

syslog 被系统首次调用的时候，他会创建一个域套接字，然后调用connect连接到由syslogd守护进程创建的uinx域套接字众所周知的路径如（/var/run/log）,这个套接字会一直打开直到进程终止。作为替换进程也可调用openlog和closelog。

	#include <syslog.h>

	void openlog(const char *ident,int options,int facility);

	void closelog(void);

openlog可以在首次调用syslog 前调用，closelog在进程不再需要发送系统消息的时候调用。

ident参数是一个由syslog冠于每个日志消息之前的字符串。它的值通常是程序的名字。

options参数由图13-3 所示的一个或多个常值的逻辑构成。

options参数由一个或多个常值的逻辑或构成

	options			说明
	LOG_CONS		若无法发送到syslogd 的守护进程则登记到控制台
	LOG_NDELAY		不延迟打开，立即创建套接字
	LOG_PERROR		即登记到syslogd守护进程，又登记到标准错误输出
	LOG_PID			随每个日志消息登记进进程ID

openlog被调用的时候通常不立即创建unix域套接字。相反直到调用syslog的时候才会打开。LOG_NDELAY设置选项会使域套接字在openlog的时候就会被创建。

openlog的facility参数为没有指定设施的后续的syslog指定一个默认值。有些人用openlog指定设施然后再用syslog时候只是指定级别。

有些消息也可以通过logger命令来产生，logger 也可以用再shell中向syslogd 发送消息。

####13.4 daemon_init函数

图13-4给出了名为daemon_init的函数，通过调用它，我们能把一个普通进程变为守护进程，该函数再unix 变体上都适用，不过有些unix变体提供了一个daemon的c库，实现类似功能，BSD和linux 均提供这个daemon函数。

fork：

	首先调用fork，然后终止父进程，留下子进程继续运行。如果本进程是
	从前台作为一个shell命令启动，当父进程终止的时候，shell就认为
	该命令已经执行完毕。这样子进程就自动再后台运行。另外，子进程继
	承了父进程的进程组id，不过他有自己的进程id。这就保证子进程不是
	一个进程组的头进程，这就是调用setsid的必要条件

setsid：

	setsid是一个posix函数，用于创建一个新的会话session。当进程变
	为新会话的会话头进程以及新进程组的进程组头进程，从而不再有进程
	控制

忽略SIGHUP信号并且再次fork：

	忽略SIGHUP信号并且再次调用fork。该函数返回的时候，父进程实际
	上是上一次调用fork产生的子进程，他被终止掉，留下新的子进程继
	续运行。再次fork的目的是为了确保本守护进程即使将来再打开一个终
	端设备，也不会自动获得控制终端。当没有控制终端的一个会话头进程
	打开一个终端设备的时候（该终端不会是当前某个其他会话的控制终
	端）。这里必须要忽略SIGHUP信号，因为当会话头进程终止时候，其会
	话中的所有进程都会收到SUGHUP信号。

这里我的理解：
首先第一次fork：这里第一次fork的作用就是让shell认为这条命令已经终止，不用挂在终端输入上;再一个是为了后面的setsid服务，因为调用setsid函数的进程不能是进程组组长(会报错Operation not permitted)，如果不fork子进程，那么此时的父进程是进程组组长，无法调用setsid。所以到这里子进程便成为了一个新会话组的组长。
第二次fork：第二次fork是为了避免后期进程误操作而再次打开终端。因为打开一个控制终端的前提条件是该进程必须为会话组组长，而我们通过第二次fork，确保了第二次fork出来的子进程不会是会话组组长，防止进程组长出现误操作导致整个进程组进程全部结束

为错误处理函数设置标识：

	把全局变量daemon_proc设置为非0值，这个变量由我们的erxxxx函
	数定义，其值非0是再告知他们改为调用syslog函数，取代fprintf标
	准错误输出。这个变量省的我们从头到尾修改程序，在服务器不是守护
	进程运行的场合调用某个错误处理函数，在服务器为守护进程的场合调
	用syslog。

改变工作目录：

	把工作目录改根目录，不过有些守护进程另有原因需要更改到其他目
	录。距离来说打印机守护进程可能改到打印机的假脱机处理（spool）
	目录。改变工作目录的另一个理由是守护进程可能是在某个任意的文件
	系统中启动，如果仍然在其中，那么文件系统可能没有办法拆卸，除非
	使用潜在错误性的强制措施。

关闭所有打开的描述符：

	关闭本守护进程从执行它的进程继承来的所有打开的文件描述符。问题
	是怎么检测到正在使用的最大的描述符，没有现成的unix函数提供检
	测。检测当前进程打开的最大文件描述符的办法，然而这个限制可以是
	无限制的，这样的检测也变得复杂我们的办法是直接关闭前64个文件描
	述符。

将stdin、stdout和stderr重定向到/dev/null

	打开/dev/null作为本守护进程的标准输入、标准输出和标准错误输
	出。这一点保证这些常用描述符是打开的，针对他们的read系统调用返
	回0（EOF），write系统调用则由内核丢弃所写的数据。打开这些文件
	描述符的理由在于，守护进程调用的那些假设能从标准输入读或者往标
	准输出或标准输出错误的输出写的库函数将不会因这些描述符未打开而
	失败。这种失败是存在隐患的，要是一个守护进程没有打开这些描述
	符，却作为客户端某个套接字的映射被打开，这种情况下如果守护进程
	调用了注入perror之类的函数，就会把非预期的数据发送给客户。

使用syslogd处理错误:


代码：

c语言版本守护进程

	#include "common.h"
	
	int daemon_proc;
	
	int daemon_init()
	{
		int i;
		pid_t pid;
	
		if ((pid = fork()) < 0)
		{
			return -1;
		}else if (pid)
		{
			exit(-1);
		}
	
		if (setsid() < 0)
		{
			return -1;
		}
	
		//忽略挂起信号 因为创建领头进程后再次调用fork，然后exit掉这次的父进程后会向进程组的前后台所有进程都发送SIGHUP信号,再次fork的目的是确保这个守护进程再次打开一个进程也不会自动获得控制终端
		signal(SIGHUP, SIG_IGN);
	
		if ((pid = fork()) < 0)
		{
			return -1;
		}
		else if (pid)
		{
			exit(-1);
		}
	
		daemon_proc = 1;
	
		chdir("/");
	
		for (i = 0; i < 64; i++)
			close(i);
	
		open("/dev/null",O_RDONLY);
		open("/dev/null", O_RDWR);
		open("/dev/null", O_RDWR);
	
		return 0;
	}
	
	int main(int argc, char** argv)
	{
		daemon_init();
	
		while (1)
		{
			
		}
	}

php版本守护进程

	<?php
	/**
	 * Description:
	 * Created by PhpStorm.
	 * User: 张磊
	 * Date: 2018/12/12
	 * Time: 12:28
	 */
	
	function daemon_init()
	{
	    if(($pid = pcntl_fork()) < 0)
	    {
	        return false;
	    }elseif($pid)
	    {
	        exit(0);
	    }
	
	    posix_setsid();
	
	    pcntl_signal(SIGHUP,SIG_IGN);
	
	    if(($pid = pcntl_fork()) < 0)
	    {
	        return false;
	    }else if($pid)
	    {
	        exit(0);
	    }
	
	    chdir("/");
	
	    fclose(STDIN);
	    fclose(STDOUT);
	    fclose(STDERR);
	
	    fopen("/dev/null","w");
	    fopen("/dev/null","rw");
	    fopen("/dev/null","rw");
	
	    return true;
	}
	
	daemon_init();
	
	
	while(1)
	{
	    sleep(1);
	}

####13.5 inetd 守护进程

典型的unix系统中可能存在许多服务器，他们只是等待客户请求，例如FTP、Telnet、Rlogin、TFTP等等

unix 系统中几乎每一个系统中的守护进程都是创建一个套接字，监听套接字请求来了之后再为每一个客户请求创建一个进程，然后继续等待下一个请求，但是这样做存在两个问题

1）所有这些守护进程含有几乎相同的代码，即表现在创建套接字，也表现在演变成守护进程上

2）每个进程在进程表中占据一个表项，然而他们大部分时候处于睡眠状态

我们使用inetd这个守护进程就可以解决上面的这两个问题

1）由inted处理普通守护进程大部分启动细节以简化守护进程的编写。这样以来每个服务器不再有调用daemon_init函数的必要

2）单个进程的inetd就能为多个服务等待外来的客户请求，以代替每个服务一个进程的做法，减少了系统中的进程总数

inetd 进程使 我们随daemon_init函数讲解技巧把自己演变成一个守护进程。接着读入自己的配置文件。通常是/etc/inetd.conf的配置文件指定本服务器的超级服务器处理哪些服务以及当一个服务请求到达时候该如何做

inetd 的工作流程    

1）
在启动阶段,读入/etc/inetd.conf文件并给该文件中指定的每个服务创建一个适当的类型套接字。inetd处理服务器的最大数目取决于inetd能创建描述符的最大数目，新创建的套接字将加入select的描述符集合中

2）为每个套接字调用bind，指定捆绑相应服务器中所周知的端口和通配地址。这个TCP或者UDP端口号通过getservbyname获得，作为函数参数的应该是配置文件中的server-name和protocol字段

3）对于每个TCP调用listen已接受外来请求，对于udp则不执行这个步骤

4）创建完所有套接字以后，调用select确保任何一个套接字变为可读，TCP将在有新连接的时候变为可读，udp将在数据报到达的时候变为可读，inetd大部分时间阻塞在select上

5）当select返回后是一个tcp套接字，而且服务器的wait-flag为nowait则调用accept

6）inetd fork 派生子进程，并且由子进程来处理请求。

子进程关闭要处理的套接字描述符之外的所有描述符；对tcp服务器来说，这个套接字是accept返回的套接字，子进程dup 2三次，把这个描述符复制到描述符0,1,2（标准输入，标准输出，标准错误输出），然后关闭原进程描述符。子进程打开的描述符于是只有0,1,2.子进程标准输入读实际是从该套接字读，标准输出或者标准错误输出实际是往该套接字上输出。子进程根据配置文件中的login-name的值，调用getpwname获取对应的保密文件表项。如果login-name不是root，子进程则调用setgid和setuid把自身改为指定的用户。

子进程然后调用exec执行相应的server-program字段指定的程序来处理具体的请求，相应的server-program-arguments字段值作为命令行参数传递给该程序。

7）如果第5补返回的是一个字节流套接字，那么父进程必须关闭已连接的套接字，然后再次select，等待下一秒变为可读的套接字。

子进程调用exec。回顾4.7，通常状态下所有描述符跨exec保持打开，因此加载exec的实际服务程序使0、1或2与客户通信，服务器应该只打开这些描述符。

上述情形使配置中指定了no-wait的标志服务器，对于tcp服务这典型的设置一位着inetd必须等待某个子进程终止就可以接受对于该子进程所提供服务的另一个连接。如果对于某个子进程所提供之服务的另一个连接确实再该子进程终止之前到达，那么一旦父进程再次调用select，这个连接就立即回到父进程。前面列出第四 第五 第六步骤再次被执行 ，于是就派生出一个子进程来处理这个请求。

给一个数据服务指定wait标志导致父进程执行步骤发生了变化。这个标志要求inetd必须在这个套接字再次称为select调用的该候选套接字之前等待当前服务该套接字的子进程终结。

发生的变化有以下几点

1）fork回到父进程的时候，父进程保存子进程的ID.这么做使得父进程能够通过查看由waitpid返回值确定子进程终止时间。

2）父进程通过FD_CLR宏关闭这个套接字在select所用的描述符集中的对应位,达成在将来的select调用中禁止这个套接字的目的。这意味着子进程将接管这个套接字直到子进程终止。

3）当子进程终止的时候，父进程被告知SIGCHILD信号，而父进程通过信号处理函数将会获得子进程id。父进程通过打开相应的套接字在select所用描述符集中的对应位，使得套接字描述符重新称为select的候选套接字。

数据服务器必须接管其套接字一直到自身终止，以防止inetd在此期间让select检查该套接字的可读性，这时因为每个数据报服务器只有一个套接字，不像tcp一样有服务器套接字 又有客户端套接字。如果inetd不关闭对于某数据报可读条件的检查，而且父进程先于服务套接字的子进程执行，那么引发本次fork的那个数据报仍然在套接字的接收缓冲区中，导致select再次返回可读，导致其再次fork另一个子进程。inetd必须在知道子进程已从套接字接收队列中读走该数据报之前忽略这个数据报套接字。inetd知道子进程何时执行完是通过SIGCHILD信号。

既然一个TCP服务器调用accept的进程使inetd，由inetd启动的真正服务器通过调用getpeername获取客户端的ip和端口。我们知道fork和exec之后，服务器熟悉客户端的唯一途径是通过getpeername。

inetd不适合web服务器或者邮箱服务器因为每一个请求需要一个进程 和fork开销太大。

####13.6 daemon_inetd函数

	#include "unp.h"
	#include "syslog.h"

	extern int daemon_proc;

	void daemon_inetd(const char *pname,int facility)
	{
		daemon_proc = 1;
		openlog(pname,LOG_PID,facility);
	}

本函数与daemon_init相比微不足道，因为守护进程已经在inetd中完成了。

	#include "unp.h"
	#include <time.h>

	int main(int argc,char **argv)
	{
		socklen_t len;
		struct sockaddr *cliaddr;
		char buf[MAXLINE];
		time_t ticks;
	
		daemon_inetd(argv[0],0);

		cliaddr = malloc(sizeof(struct sockaddr_storage));

		len = sizeof(struct sockaddr_storage);

		getpeername(0,cliaddr,&len);

		printf("connection from %s\n",sock_ntop(cliaddr,len));

		ticks = time(NULL);

		snprintf(buff,sizeof(buff),"%.24s\r\n",ctime(&ticks));

		write(0,buff,sizeof(buff));

		close(0);

		exit(0);
	}

这段程序有两大改动。首先所有套接字创建代码（即tcp_listen和accept的调用都消失了）。这些步骤由inetd执行，我们使用0描述符指代已由inetd接受tcp连接。无线循环的for循环也消失了，因为本服务器程序针对每个客户只启动一次。服务完当前客户程序终止。


####13.7小结

守护进程是在后台运行并独立于终端控制的进程，许多网络服务器作为守护进程来运行。守护进程的所有输出都通过调用syslog函数发送给syslogd守护进程。系统管理员可以根据发送消息的守护进程以及消息的严重级别，完全控制这些消息的处理方式。

创建一个守护进程需要以下步骤：

fork转到后台运行，调用setsid建立一个新的POSIX会话头进程，再次fork以避免无意中获得新的控制终端，改变工作目录和文件创建模式掩码，关闭所有非必要的描述符。我们的daemon_init处理这些细节。

许多服务器由inetd启动服务器，它简化了服务器的启动步骤，当启动真正的服务器的时候，套接字已经在标准输入、标准输出和标准错误输出上打开。这样我们不需要再调用socket、bind、listen、accept，因为这些步骤已经由inetd 处理