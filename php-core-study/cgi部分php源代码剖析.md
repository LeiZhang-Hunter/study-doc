#fastcgi 部分php 源码剖析

##信号的安装（fast_sapi的开始在cgi_main.c）

看到第一处令我疑惑的内容，在这里声明了一个空指针，引起了我的思考这个空指针是做什么用的呢？
	
	fcgi_request *request = NULL;
	
于是我在fastcgi.h找到了这个内容的实现

	typedef struct _fcgi_request fcgi_request;
	
继续找到fastcgi.c 在这里面看到了具体的内容

	struct _fcgi_request {
		int            listen_socket;
		int            tcp;
		int            fd;
		int            id;
		int            keep;
	#ifdef TCP_NODELAY
		int            nodelay;
	#endif
		int            ended;
		int            in_len;
		int            in_pad;

		fcgi_header   *out_hdr;

		unsigned char *out_pos;
		unsigned char  out_buf[1024*8];
		unsigned char  reserved[sizeof(fcgi_end_request_rec)];

		fcgi_req_hook  hook;

		int            has_env;
		fcgi_hash      env;
	};
	
分析一下这个字段 listen_socket 这个是监听的fd 描述符

	#ifdef TCP_NODELAY
		int            nodelay;
	#endif

这个是纳格算法，这里我要说一下tcp拥塞控制以及滑动窗口了，因为高速io的时候，会出现丢包这类情况，所以要对传输进行控制，因为tcp有ack所以丢包不丢包很容易控制，我们公司的大牛在降tcp那些事的时候说过，tcp有慢启动的过程，在启动之初会有一个cwnd值来控制传输速度，收发成功一个cwnd就会+1，然后当出现丢包的时候会/2，这样来控制拥塞，为了避免拥塞算法还有好多tcp算法，纳格算法就是其中一个

	fcgi_req_hook  hook;
	
其中的结构体实现在

	typedef struct _fcgi_req_hook 	fcgi_req_hook;

	struct _fcgi_req_hook {
		void(*on_accept)();
		void(*on_read)();
		void(*on_close)();
	};
	
定义了一些回调的钩子在不同场景下使用，比如说on_read在套接字可读的时候用on_read,要关闭的时候调用on_close,在有可以链接的fd的时候会调用accept


继续往下读 读到 

	#ifdef HAVE_SIGNAL_H
	#if defined(SIGPIPE) && defined(SIG_IGN)
		signal(SIGPIPE, SIG_IGN); /* ignore SIGPIPE in standalone mode so
									that sockets created via fsockopen()
									don't kill PHP if the remote site
									closes it.  in apache|apxs mode apache
									does that for us!  thies@thieso.net
									20000419 */
	#endif
	#endif
	
这里是做信号处理的，如果有这个宏 signal SIGPIPE，忽略掉这个信号，为什么要做这个处理呢？"unix网络编程上卷中"对这个信号单独拿出来一节描述,如果对一个已经关闭的描述符进行多次写入，就会产生SIGPIPE这个信号，如果不做处理就会默认终止程序的运行，为了防止程序运行，这里调用了SIG_IGN用来忽略掉这个信号，防止程序exit掉。

我们再看一下zend对信号的主要处理函数

	zend_signal_startup();
	
不要小看这一行函数，这一行函数里有非常多的知识点！！！

	/* {{{ zend_signal_startup
	 * alloc zend signal globals */
	ZEND_API void zend_signal_startup(void)
	{

	#ifdef ZTS
		ts_allocate_id(&zend_signal_globals_id, sizeof(zend_signal_globals_t), (ts_allocate_ctor) zend_signal_globals_ctor, NULL);
	#else
		zend_signal_globals_ctor(&zend_signal_globals);
	#endif

		/* Used to block signals during execution of signal handlers */
		sigfillset(&global_sigmask);
		sigdelset(&global_sigmask, SIGILL);
		sigdelset(&global_sigmask, SIGABRT);
		sigdelset(&global_sigmask, SIGFPE);
		sigdelset(&global_sigmask, SIGKILL);
		sigdelset(&global_sigmask, SIGSEGV);
		sigdelset(&global_sigmask, SIGCONT);
		sigdelset(&global_sigmask, SIGSTOP);
		sigdelset(&global_sigmask, SIGTSTP);
		sigdelset(&global_sigmask, SIGTTIN);
		sigdelset(&global_sigmask, SIGTTOU);
	#ifdef SIGBUS
		sigdelset(&global_sigmask, SIGBUS);
	#endif
	#ifdef SIGSYS
		sigdelset(&global_sigmask, SIGSYS);
	#endif
	#ifdef SIGTRAP
		sigdelset(&global_sigmask, SIGTRAP);
	#endif

		zend_signal_init();
	}
	
非线程安全下的逻辑我们只看这里

	zend_signal_globals_ctor(&zend_signal_globals);
	
具体源码实现：

	static void zend_signal_globals_ctor(zend_signal_globals_t *zend_signal_globals) /* {{{ */
	{
		size_t x;

		memset(zend_signal_globals, 0, sizeof(*zend_signal_globals));

		for (x = 0; x < sizeof(zend_signal_globals->pstorage) / sizeof(*zend_signal_globals->pstorage); ++x) {
			zend_signal_queue_t *queue = &zend_signal_globals->pstorage[x];
			queue->zend_signal.signo = 0;
			queue->next = zend_signal_globals->pavail;
			zend_signal_globals->pavail = queue;
		}
	}
	
我们看一下这个函数走了什么memset初始化了zend_signal_globals这个结构体,这个结构体的真实构造是

	typedef struct _zend_signal_globals_t {
		int depth;
		int blocked;            /* 1==TRUE, 0==FALSE */
		int running;            /* in signal handler execution */
		int active;             /* internal signal handling is enabled */
		zend_bool check;        /* check for replaced handlers on shutdown */
		zend_signal_entry_t handlers[NSIG];
		zend_signal_queue_t pstorage[ZEND_SIGNAL_QUEUE_SIZE], *phead, *ptail, *pavail; /* pending queue */
	} zend_signal_globals_t;

depth在php源代码中我没法先有使用操作他加减的宏是ZEND_SIGNAL_BLOCK_INTERRUPTIONS和ZEND_SIGNAL_UNBLOCK_INTERRUPTIONS

	# ifdef ZTS
	#  define ZEND_SIGNAL_BLOCK_INTERRUPTIONS() if (EXPECTED(zend_signal_globals_id)) { SIGG(depth)++; }
	#  define ZEND_SIGNAL_UNBLOCK_INTERRUPTIONS() if (EXPECTED(zend_signal_globals_id) && UNEXPECTED(((SIGG(depth)--) == SIGG(blocked)))) { zend_signal_handler_unblock(); }
	# else /* ZTS */
	#  define ZEND_SIGNAL_BLOCK_INTERRUPTIONS()  SIGG(depth)++;
	#  define ZEND_SIGNAL_UNBLOCK_INTERRUPTIONS() if (((SIGG(depth)--) == SIGG(blocked))) { zend_signal_handler_unblock(); }
	# endif /* not ZTS */
	
这个宏是十分重要的宏 下面我们看他的实现

ZEND_SIGNAL_BLOCK_INTERRUPTIONS 给 信号延迟深度+1

ZEND_SIGNAL_UNBLOCK_INTERRUPTIONS 给信号深度-1

这是一个十分重要的宏，我们在下面会对这个宏进行深度分析，来介绍这个宏的精妙之处

blocked 是是否阻塞

running运行状态位

zend_signal_queue_t pstorage[ZEND_SIGNAL_QUEUE_SIZE], *phead, *ptail, *pavail; /* pending queue */这个是等待处理的信号队列

我们在看一下 zend_signal_queue_t 队列中每个单元的具体定义

	typedef struct _zend_signal_queue_t {
		zend_signal_t zend_signal;
		struct _zend_signal_queue_t *next;
	} zend_signal_queue_t;
	
我们可以看出了zend_signal_t zend_signal这个是要出里的信号,next是个链表，为什么要这么做呢，因为hash冲突啊！！！如果多个处理函数被映射到一个信号上就需要链表了！！！

还有一个很重要的栈zend_signal_entry_t handlers[NSIG];每一个值是一个结构体zend_signal_t，下面我们看一下这个结构体的定义

	typedef struct _zend_signal_t {
		int signo;
		siginfo_t *siginfo;
		void* context;
	} zend_signal_t;

综上所属zend_signal_globals_ctor这个函数就是初始化zend_signal_globals这个结构体以及这个结构体里的信号处理队列

下面我们更加要看一下zend_signal_startup 这里面的一些至关重要的c语言函数，sigfillset 以及 sigdelset 这两个函数在unix高级环境编程第10章第11节的信号集这一章节中有明确的介绍，下面我回顾一下书中的内容

####信号集（unix 高级环境编程中的信号集回顾）

我们需要有一个能表示多个----信号集的数据类型

几个重要的信号集函数

	#include <signal.h>
	int sigemptyset(sigset_t *set);
	int sigfillset(sigset_t *set);
	int sigaddset(sigset_t *set,int signo);
	int sigdelset(sigset_t *set,int signo);
	int sigismember(const sigset_t *set,int signo);
	
第一个函数sigemptyset这个函数是用来初始化set指向的信号集的，清除信号集中的所有信号，毕竟函数就像他的名字empty空，这个sigemptyset 设置信号集为空
第二个函数sigfillset初始化信号集，使他包含所有信号
所有信号在使用信号集之前要调用sigemptyset或者sigfillset一次
下面的三个函数是对信号集的增删改查
第三个函数sigaddset(sigset_t *set,int signo);添加一个信号入信号集
第四个函数sigdelset(sigset_t *set,int signo);删除一个信号入信号集
第五个函数sigismember(sigset_t *set,int signo);检查信号集中是否有这个信号

好到了这里我们继续分析zend_signal_startup这个函数，

	sigfillset(&global_sigmask);

初始化信号集，把所有信号都加入到了信号集中

	sigdelset
	
在一次调用这个函数从信号集中删除不用的信号，我们再看一下删除的着一些信号都是什么！（注意linux下面有可靠信号和非可靠信号的区别）

	SIGILL    无效的程序映像，如无效指令
	SIGABRT  异常终止条件，如例如由中止（）
	SIGFPE  错误的算术运算，如除以零（这个我之前遇到过比如说整数除以0直接会dump掉，哈哈哈哈）
	SIGKILL  这个是发送关闭信号 kill -9 常用信号 不需要多说
	SIGSEGV   段错误不需要多说
	SIGCONT  作业控制信号，默认含义是进程停止后继续运行
	SIGSTOP  作业控制信号，用于停止一个进程	
	SIGTSTP 用户按ctrl z 默认停止进程的交互执行
	SIGTTIN 一个后台进程组中的进程试图控制终端时候，终端驱动程序会产生这个信号
	SIGTTOU 后台控制组进程试图写到终端的时候会触发这个信号
	
最后一行运行的函数zend_signal_init，我们看一下他的具体实现

	void zend_signal_init(void) /* {{{ */
	{
		int signo;
		struct sigaction sa;

		/* Save previously registered signal handlers into orig_handlers */
		memset(&global_orig_handlers, 0, sizeof(global_orig_handlers));
		for (signo = 1; signo < NSIG; ++signo) {
			if (sigaction(signo, NULL, &sa) == 0) {
				global_orig_handlers[signo-1].flags = sa.sa_flags;
				if (sa.sa_flags & SA_SIGINFO) {
					global_orig_handlers[signo-1].handler = (void *) sa.sa_sigaction;
				} else {
					global_orig_handlers[signo-1].handler = (void *) sa.sa_handler;
				}
			}
		}
	}
	
sigaction 类似与signal，但是可以做更多的事情,将安装的信号处理器global_orig_handlers里面的存储值初始化

我们在main.c 的 php_request_startup 里看到一个函数zend_signal_activate 这是zend signal 机制的激活函数 下面我们来分析一下zend_signal_activate 这个函数的作用

	void zend_signal_activate(void)
	{
		size_t x;

		memcpy(&SIGG(handlers), &global_orig_handlers, sizeof(global_orig_handlers));

		for (x = 0; x < sizeof(zend_sigs) / sizeof(*zend_sigs); x++) {
			zend_signal_register(zend_sigs[x], zend_signal_handler_defer);
		}

		SIGG(active) = 1;
		SIGG(depth)  = 0;
		SIGG(check)  = ZEND_DEBUG;
	}

首先这个函数会依次将zend_sigs中的信号通过zend_signal_register注册到zend的信号机制中，并且绑定zend_signal_handler_defer这个函数，激活状态位设置为1（active）,depth设置为0,调试设置为关闭

这里出现了两个非常非常关键的函数zend_signal_register和zend_signal_handler_defer下面我门来一起分析一下zend_signal_handler_defer这个函数做了什么

	static int zend_signal_register(int signo, void (*handler)(int, siginfo_t*, void*))
	{
		struct sigaction sa;

		if (sigaction(signo, NULL, &sa) == 0) {
			if ((sa.sa_flags & SA_SIGINFO) && sa.sa_sigaction == handler) {
				return FAILURE;
			}

			SIGG(handlers)[signo-1].flags = sa.sa_flags;
			if (sa.sa_flags & SA_SIGINFO) {
				SIGG(handlers)[signo-1].handler = (void *)sa.sa_sigaction;
			} else {
				SIGG(handlers)[signo-1].handler = (void *)sa.sa_handler;
			}

			sa.sa_flags     = SA_SIGINFO; /* we'll use a siginfo handler */
			sa.sa_sigaction = handler;
			sa.sa_mask      = global_sigmask;

			if (sigaction(signo, &sa, NULL) < 0) {
				zend_error_noreturn(E_ERROR, "Error installing signal handler for %d", signo);
			}

			return SUCCESS;
		}
		return FAILURE;
	}
	
还是要具体分析一下这里面的函数的作用，sigaction

这个函数sigaction，在“unix高级环境编程”第十章，第14小节中进行了十分详细的讲述，书中的介绍是：

sigaction函数的功能是检查或修改与指定信号相关联的处理动作，注意有两个功能 ！检查和！修改，相对于signal来说可以是更加好用。

	#include <signal.h>
	int sigaction(int signo,const struct sigaction* restrict act,struct sigaction *restrict oact);
	
其中参数signo是要检查或者修改的信号编号，如果act指针不是空的，那么就是要进行修改他的动作，如果oact是非空的指针就代表是要返回这个信号的上一个动作。

sigaction 这个结构体我认为也十分有必要剖析一下，但是内容比较多详细见<unix高级环境编程> 第10章 第 14小节

函数中第一行 

	if (sigaction(signo, NULL, &sa) == 0) {
		if ((sa.sa_flags & SA_SIGINFO) && sa.sa_sigaction == handler) {
			return FAILURE;
		}
	
这里是获取这个信号的上一个动作,先说一下SA_SIGINFO这个标志位，这个标志位书上说这说明返回的sa这个指针是一个指向siginfo结构的指针以及一个指向进程上下文标识符的指针,sa_sigaction是一个替代的信号处理程序，当在sigaction结构中使用了SA_SIGINFO,就要使用sa_sigaction

也就说说zend_signal_register这个函数的作用是检查这个信号是否注册过注册过返回false，没有注册过注册到zend_signal_globals的handlers这个结构体变量中，然后执行sigaction把这个信号安装给内核。


我们再看zend内核中的另一个函数zend_signal_handler_defer，这也是一个最核心的函数


	void zend_signal_handler_defer(int signo, siginfo_t *siginfo, void *context)
	{
		int errno_save = errno;
		zend_signal_queue_t *queue, *qtmp;
		zend_bool is_handling_safe = 1;

	#ifdef ZTS
		ZEND_TSRMLS_CACHE_UPDATE();
		/* A signal could hit after TSRM shutdown, in this case globals are already freed. */
		if (NULL == TSRMLS_CACHE || NULL == TSRMG_BULK_STATIC(zend_signal_globals_id, zend_signal_globals_t *)) {
			is_handling_safe = 0;
		}
	#endif

		if (EXPECTED(is_handling_safe && SIGG(active))) {
			if (UNEXPECTED(SIGG(depth) == 0)) { /* try to handle signal */
				if (UNEXPECTED(SIGG(blocked))) {
					SIGG(blocked) = 0;
				}
				if (EXPECTED(SIGG(running) == 0)) {
					SIGG(running) = 1;
					zend_signal_handler(signo, siginfo, context);

					queue = SIGG(phead);
					SIGG(phead) = NULL;

					while (queue) {
						zend_signal_handler(queue->zend_signal.signo, queue->zend_signal.siginfo, queue->zend_signal.context);
						qtmp = queue->next;
						queue->next = SIGG(pavail);
						queue->zend_signal.signo = 0;
						SIGG(pavail) = queue;
						queue = qtmp;
					}
					SIGG(running) = 0;
				}
			} else { /* delay signal handling */
				SIGG(blocked) = 1; /* signal is blocked */

				if ((queue = SIGG(pavail))) { /* if none available it's simply forgotton */
					SIGG(pavail) = queue->next;
					queue->zend_signal.signo = signo;
					queue->zend_signal.siginfo = siginfo;
					queue->zend_signal.context = context;
					queue->next = NULL;

					if (SIGG(phead) && SIGG(ptail)) {
						SIGG(ptail)->next = queue;
					} else {
						SIGG(phead) = queue;
					}
					SIGG(ptail) = queue;
				}
	#if ZEND_DEBUG
				else { /* this may not be safe to do, but could work and be useful */
					zend_output_debug_string(0, "zend_signal: not enough queue storage, lost signal (%d)", signo);
				}
	#endif
			}
		} else {
			/* need to just run handler if we're inactive and getting a signal */
			zend_signal_handler(signo, siginfo, context);
		}

		errno = errno_save;
	}
	
我们看一个宏的实现EXPECTED 具体实现，

	#if PHP_HAVE_BUILTIN_EXPECT
	# define EXPECTED(condition)   __builtin_expect(!!(condition), 1)
	# define UNEXPECTED(condition) __builtin_expect(!!(condition), 0)
	#else
	# define EXPECTED(condition)   (condition)
	# define UNEXPECTED(condition) (condition)
	#endif
	
我们应该关注一下__builtin_expect 这个函数的作用

这个指令是gcc引入的，作用是允许程序员将最有可能执行的分支告诉编译器。这个指令的写法为：__builtin_expect(EXP, N)。
意思是：EXP==N的概率很大。__builtin_expect() 是 GCC (version >= 2.96）提供给程序员使用的，目的是将“分支转移”的信息提供给编译器，这样编译器可以对代码进行优化，以减少指令跳转带来的性能下降。

也就是说是很简答的条件判断，目的是为了优化编译器的速度，可以直接无视的
		
这里是判断zend全局信号处理是否被激活

		if (EXPECTED(is_handling_safe && SIGG(active))) {
		
如果zend的信号处理器被激活了，而且是安全的

	if (UNEXPECTED(SIGG(depth) == 0)) { /* try to handle signal */
		if (UNEXPECTED(SIGG(blocked))) {
				SIGG(blocked) = 0;
			}
	}
	
如果说SIGG(depth) 有很大概率是0，SIGG(blocked) ==ZEND_SIGNAL_UNBLOCK_INTERRUPTIONS 0;

	if (EXPECTED(SIGG(running) == 0)) {
			SIGG(running) = 1;
			zend_signal_handler(signo, siginfo, context);

			queue = SIGG(phead);
			SIGG(phead) = NULL;

			while (queue) {
				zend_signal_handler(queue->zend_signal.signo, queue->zend_signal.siginfo, queue->zend_signal.context);
				qtmp = queue->next;
				queue->next = SIGG(pavail);
				queue->zend_signal.signo = 0;
				SIGG(pavail) = queue;
				queue = qtmp;
			}
			SIGG(running) = 0;
		}

如果说信号处理器有很大概率没有运行，running运行位被设置为1，然后我们看到zend_signal_handler(signo, siginfo, context);重点分析一下zend_signal_handler这个函数的实现

	/* {{{ zend_signal_handler
	 *  Call the previously registered handler for a signal
	 */
	static void zend_signal_handler(int signo, siginfo_t *siginfo, void *context)
	{
		int errno_save = errno;
		struct sigaction sa;
		sigset_t sigset;
		zend_signal_entry_t p_sig;
	#ifdef ZTS
		if (NULL == TSRMLS_CACHE || NULL == TSRMG_BULK_STATIC(zend_signal_globals_id, zend_signal_globals_t *)) {
			p_sig.flags = 0;
			p_sig.handler = SIG_DFL;
		} else
	#endif
		p_sig = SIGG(handlers)[signo-1];

		if (p_sig.handler == SIG_DFL) { /* raise default handler */
			if (sigaction(signo, NULL, &sa) == 0) {
				sa.sa_handler = SIG_DFL;
				sigemptyset(&sa.sa_mask);

				sigemptyset(&sigset);
				sigaddset(&sigset, signo);

				if (sigaction(signo, &sa, NULL) == 0) {
					/* throw away any blocked signals */
					zend_sigprocmask(SIG_UNBLOCK, &sigset, NULL);
	#ifdef ZTS
	# define RAISE_ERROR "raise() failed\n"
					if (raise(signo) != 0) {
						/* On some systems raise() fails with errno 3: No such process */
						kill(getpid(), signo);
					}
	#else
					kill(getpid(), signo);
	#endif
				}
			}
		} else if (p_sig.handler != SIG_IGN) {
			if (p_sig.flags & SA_SIGINFO) {
				if (p_sig.flags & SA_RESETHAND) {
					SIGG(handlers)[signo-1].flags   = 0;
					SIGG(handlers)[signo-1].handler = SIG_DFL;
				}
				(*(void (*)(int, siginfo_t*, void*))p_sig.handler)(signo, siginfo, context);
			} else {
				(*(void (*)(int))p_sig.handler)(signo);
			}
		}

		errno = errno_save;
	}

我们先来分析这一行

		p_sig = SIGG(handlers)[signo-1];
		
是从处理池子中拿出对应的处理函数

我们在这里可以看到一个宏叫SIG_DFL，在unix高级环境编程中“10.3”章节中我们可以找到这几个宏的身影，signal函数中对几个宏的实现有具体的介绍

	#define SIG_ERR (void(*)())-1  
	#define SIG_DFL (void(*)())0
	#define SIG_IGN (void(*)())1
	
这些常量可以用于代替“指向函数的指针，该函数需要一个整形参数，而且无返回值”。signal的第二个参数可以用于保存他们,书中对SIG_DFL和SIG_IGN做了介绍


	SIG_DFL 有点想default  代表对信号采用系统默认的处理方式 比如说SIGTERM默认的是关闭信号
	SIG_IGN 这个毋庸置疑就是不处理信号
	SIG_ERR 是出错的返回

单独拉出下面这段代码继续分析

		if (p_sig.handler == SIG_DFL) { /* raise default handler */
			if (sigaction(signo, NULL, &sa) == 0) {
				sa.sa_handler = SIG_DFL;
				sigemptyset(&sa.sa_mask);

				sigemptyset(&sigset);
				sigaddset(&sigset, signo);

				if (sigaction(signo, &sa, NULL) == 0) {
					/* throw away any blocked signals */
					zend_sigprocmask(SIG_UNBLOCK, &sigset, NULL);
	#ifdef ZTS
	# define RAISE_ERROR "raise() failed\n"
					if (raise(signo) != 0) {
						/* On some systems raise() fails with errno 3: No such process */
						kill(getpid(), signo);
					}
	#else
					kill(getpid(), signo);
	#endif
				}
			}
		} 
		
这一段又出现了一个文中并没有介绍的函数zend_sigprocmask，我们看一下这个函数的实现
	
	# define zend_sigprocmask(signo, set, oldset) sigprocmask((signo), (set), (oldset))
	
我们看一下sigprocmask这个函数，在“unix高级环境编程”10.12中对这个函数做了讲解

10.8节曾经提到一个进程的信号屏蔽了当前阻塞而不能传递给这个进程的信号集。调用函数sigprocmask可以检测或者更改其信号的屏蔽字，或者在同一个步骤中同时执行这两个操作

	#include 	<signal.h>
	int sigprocmask(int how,const sigset_t *restrict set,sigset_t *restrict oset);
	
对这个函数的几个参数做一下说明:
首先，如果oset是非空的指针，那么进程的当前信号屏蔽集通过oset返回
如果set是一个非空指针，则参数how指示如何修改当前的信号屏蔽字。

比较关键的是这个函数的第一个参数how这里又有3个标志位

SIG_BLOCK 这个进程新的信号屏蔽字是其当前信号屏蔽字当前信号屏蔽字和set指向信号集的并集。set包含了我们希望阻塞的附加信号

SIG_UNBLOCK 该进程新的信号屏蔽字是当前信号屏蔽字和set所指向信号集补集的交集。set包含了我们希望解除的信号集

SIG_SETMASK 这个进程新的信号屏蔽字将被set指向的信号集代替。

我们看一下信号集的实现

	typedef struct
	{
	  unsigned long int __val[_SIGSET_NWORDS];
	} __sigset_t;

key是信号

我也写了一个demo 分享在这里同时也做进一步的说明

	#include <stdio.h>
	#include <signal.h>
	#include <stdlib.h>
	#include <fcntl.h>
	#include <unistd.h>
	int main()
	{
	    int signo;
	    int res;
	    sigset_t set;
	    sigset_t oldmask;  
	    sigemptyset(&set);   //初始化一个信号集
	    sigaddset(&set,SIGUSR1);  //加入SIGUSR1进入这个信号集
	    sigaddset(&set,SIGUSR2); //加入SIGUSR2进入这个信号集
	    sigprocmask(SIG_BLOCK,&set,&oldmask);//加入SIGUSR2进入这个信号集
	    pid_t pid = getpid();//获取进程当前pid
	    //非常有意思了这个程序一直陷入一个死循环
	    //阻塞了SIGUSR1和SIGUSR2这两个附加信号
	    while (1) {
		kill(pid,SIGUSR1);
		sigwait(&set, &signo);
		printf("hello sig:%d\n", signo);
	    }
	}
	
经过我们的代码学习zend_signal_handler的函数已经很容易理解了，检查是否是内核默认的处理方式，如果是内核默认的处理方式，则将信号的处理方式改为siginfo_t *siginfo的地址，然后在对进程发信号，从而触发异步处理，也就是zend_signal_handler这个函数是一个触发动作

我们继续来看zend_signal_handler_defer

		if (EXPECTED(is_handling_safe && SIGG(active))) {
			if (UNEXPECTED(SIGG(depth) == 0)) { /* try to handle signal */
				if (UNEXPECTED(SIGG(blocked))) {
					SIGG(blocked) = 0;
				}
				if (EXPECTED(SIGG(running) == 0)) {
					SIGG(running) = 1;
					zend_signal_handler(signo, siginfo, context);

					queue = SIGG(phead);
					SIGG(phead) = NULL;

					while (queue) {
						zend_signal_handler(queue->zend_signal.signo, queue->zend_signal.siginfo, queue->zend_signal.context);
						qtmp = queue->next;
						queue->next = SIGG(pavail);
						queue->zend_signal.signo = 0;
						SIGG(pavail) = queue;
						queue = qtmp;
					}
					SIGG(running) = 0;
				}
			} else { /* delay signal handling */
				SIGG(blocked) = 1; /* signal is blocked */

				if ((queue = SIGG(pavail))) { /* if none available it's simply forgotton */
					SIGG(pavail) = queue->next;
					queue->zend_signal.signo = signo;
					queue->zend_signal.siginfo = siginfo;
					queue->zend_signal.context = context;
					queue->next = NULL;

					if (SIGG(phead) && SIGG(ptail)) {
						SIGG(ptail)->next = queue;
					} else {
						SIGG(phead) = queue;
					}
					SIGG(ptail) = queue;
				}
	#if ZEND_DEBUG
				else { /* this may not be safe to do, but could work and be useful */
					zend_output_debug_string(0, "zend_signal: not enough queue storage, lost signal (%d)", signo);
				}
	#endif
			}
		} 
		
这一段代码的意思是当信号的深度depth是0的时候如果zend_signal没有运行则把running设置为1，然后触发当前的信号以及信号处理函数，然后循环处理队列中没有触发的信号以及处理函数，如果非0的时候会把当前要处理的信号加入到zend_signal_globals的队列之中，等待处理

这个函数起到什么作用呢？起到了延迟执行信号处理函数的作用，但是这又有什么作用呢？现在还不到得出答案的时候

我们刚才说到
	
	#  define ZEND_SIGNAL_UNBLOCK_INTERRUPTIONS() if (((SIGG(depth)--) == SIGG(blocked))) { zend_signal_handler_unblock(); }
	
这个宏要一会儿再分析，现在我们就对这个宏做出最后的分析，也是zend源码信号机制的精妙之处的体现，在之前我们已经对主要的信号都绑定过zend_signal_handler_defer这个函数，当调用ZEND_SIGNAL_BLOCK_INTERRUPTIONS这个宏的时候，depth>0 期间发生信号中断会调用函数zend_signal_handler_defer，这个时候不会触发任何事件处理器，这时候发生的事件只会增加到zend_signal_globals_t的队列里，当最后ZEND_SIGNAL_UNBLOCK_INTERRUPTIONS将depth设置为0的时候，才会真正触发刚才注入队列的事件，以此来实现了一个信号延迟执行的目的

####流程汇总

zend signal 我们这就分析完了所有的zend signal运行函数，下面我用流程图简要说明zend signal的完整运行流程


