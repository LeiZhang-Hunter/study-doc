#Posix IPC 

####2.1概述

以下三种类型的ipc合称为“POSIX IPC”

以下三种类型的ipc合称为“Posix IPC”

	Posix消息队列
	Posix信号量
	Posix共享内存
	
posix IPC的所有函数:

			消息队列		信号量				共享内存
	头文件	<mqueue.h>	<semaphore.h>	<sys/mman.h>

	创建、   mq_open		sem_open			shm_open
	打开	mq_close		sem_close			shm_unlink
	或者	mq_unlink		sem_unlink
	删除					sem_init
	的函					sem_destroy
	数
	
	控制	mq_getattr                                 ftruncate
	IPC的	mq_setattr						 	   fstat
	操作
	函数
	
	
	IPC      mq_send                    sem_wait                     mmap
	操作    mq_receive              sem_trywait                 munmap
	            mq_notify                 sem_post 
	                                                  sem_getvalue
	                                        
####IPC的名字

三种类型的posix IPC都使用POSIX IPC的名字进行标识。mq_open、sem_open和shm_open这三个函数的第一个参数就是这样的一个名字，它可能是某个文件系统中的一个真正的路径名，也可能不是，posix.1就是这么描述posix的ipc名字的.

它必须复合已有路径名规则（必须最多由PATH_MAX个字节构成，包括尾部的空字节）.如果它以斜杠符号开头，那么对这些函数的不通调用将访问同一个队列

如果它以斜杠符号开头，那么对这些函数的不同调用将访问同一个队列。如果它不以斜杠符号开头，那么效果取决于现实.

名字中额外以斜杠符号的解释由现实定义.

因此，为了方便移植，POSIX IPC的名字必须用一个斜杠符号开头，并且不能再包含有任何其他的斜杠符号.遗憾的是这些规则还不够，仍然会出现移植性问题.

solaris 2.6要求有斜杠开头，但是不允许有另外的斜杠符打头，并且不能再含有任何其他的斜杠。遗憾的是这些规则还是不够的，仍然会出现移植性问题。

Solaris2.6 要求有斜杠打头，但是不允许有另外的斜杠，假设要创建一个消息队列，创建函数在/tmp中创建三个用.MQ开头的文件。例如给mq_open的纯属为/queue.1234,那么这三个文件分别为/tmp/.MQDqueue.1234,那么这三个文件分别是/tmp/.MQDqueue.1234、/tmp/.MQLqueue.1234和/tmp/.MQPqueue.1234。

当我们指定一个只有单个斜杠（作为首字符）的名字的时候，移植性问题就发生了：我们必须在根目录中具有写权限。例如,/tmp.1234复合Posix规则，在Solaris下也行，但是在Digttal unix却会试图创建这个文件，这时候处分我们在根目录中有写权限，负责这样做将会尝试失败。如果我们指定一个/tmp/.test.1234这样的名字，那么在该名字创建一个之中文件的所有系统上都将成功。前提是/tmp目录存在，而且我门在这个目录上有写权限，对于大多数linux都是正常情况,在solaris下就会失败.

为了避免这些移植性问题，我们应该把posix ipc的名字的#define 放在一个便于秀爱的头文件之中，这样在移植操作系统的时候就需要修改这个头文件

posix.1定义了下面三个宏

S_TYPEISMQ(buf)
S_TYPEISSEM(buf)
S_TYPEISSHM(buf)

他们单个参数是指向某个stat结构的指针，其内容由fstat、lstat或stat这三个函数填入。如果制定的ipc对象（消息队列，信号量或者是共享内存）是作为一个独特的文件类型实现的，而且参数指向的stat结构访问这样的文件类型，那么这三个宏计算出一个非0值，否则计算出的值为0.

px_ipc_name函数

解决上述问题的另一种办法是自定义一个px_ipc_name的函数，它为定位posix ipc名字而添加上正确的前缀目录

	#include "unpipc.h"
	char *px_ipc_name(const char *name);
	
本书中我们给自己定义的非标准系统函数都使用这样的版式：围绕函数圆形和返回值的方框都是虚框，name参数不能有任何的斜杠

	px_ipc_name("test1");
	
	char *px_ipc_name(char* name)
	{
	    char *dir,*dst,*slash;

	    if((dst = malloc(PATH_MAX)) == NULL)
	    {
		return  NULL;
	    }

	    if((dir = getenv("PX_IPC_NAME")) == NULL) {
	#ifdef  POSIX_IPC_PREFIX
		dir = POSIX_IPC_PREFIX;
	#else
		dir = "/tmp/";
	#endif
	    }

    slash = (dir[strlen(dir)-1] == '/') ? "" : "/";

    snprintf(dst,PATH_MAX,"%s%s%s",dir,slash,name);

    return dst;
}
	                                                  
###创建与打开ipc通道

mq_open、sem_open和shm_open这三个创建或打开一个ipc对象的函数，他们的名字为oflag的第二个参数制定怎么样打开所请求的对象。这与标准open函数的第二个参数类似.图2-3给出了可组合构成这个参数的各种常值.

前三行制定怎么样打开对象：只读、只写或读写.消息队列能以其中任何一种模式打开，信号量的打开不指定任何模式（任意信号量的操作，都需要读写访问权限），共享内存对象则不能用只写模式打开.

说明:

mq_open

可以使用

只读：O_RDONLY
只写：O_WRONLY
读写：O_RDWR
非阻塞模式，如果已经存在就截断：O_NONBLOCK

sem_open:
不能使用任何指定模式

shm_open
不能用只写模式打开可以使用只读或者读写模式打开O_RDONLY,O_RDWR

非阻塞模式，如果已经存在就截断：O_TRUNC

三个函数如果不存在则创建的排他性创建
O_CREAT
O_EXCL

####O_CREAT：
O_CREAT若不存在则创建由函数第一个参数制定名字的消息队列，信号量或者共享内存区的对象，则创建一个新的消息队列、信号量或者共享内存的时候，至少需要另外一个称为mode的参数.这个参数指定权限位

常值
S_IRUSR		用户（属主）读
S_IWUSR		用户（属主）写

S_IRGRP		（属）组成员读
S_IWGRP		（属）组成员写

S_IROTH	其他用户读
S_IWOTH	其他用户写

这些常值定义在<sys/stat.h>中，这个参数使用umask函数或者shell的umask命令来设置.

跟创建新的文件一样，当创建一个新的消息队列、信号量或者共享内存区对象时候，其用户id被设置为当前进程的有效用户id。信号量或共享内存区对象的组id被设置为当前进程的有效组id或者某个系统默认的id。新的消息队列对象的组id被设置为当前进程的有效组id.

####O_EXCL:

如果这个标志和O_CREAT一起指定，那么ipc函数只在指定名字的消息队列，信号量或者共享内存对象不存在的时候才会创建新的对象。如果这个对象已经存在，而且指定了O_CREAT|o_excl，那么他会返回一个EEXIST的错误

主要问题是考虑到其他进程的存在，检查锁指定名字的消息队列、信号量或共享内存区对象的存在和创建它这两个操作必须是原子的。

####O_NONBLOCK:
这个标志使得一个消息队列在队列为空或队列填满的时候不再会被阻塞，我们会在mq_receive和mq_send里详细讨论这个标志

####O_TRUNC:

如果用只读模式打开一个已经存在的共享内存对象，那么这个标志将会使对象的长度被截断为0

图2.5 展示了打开一个ipc的真正的逻辑流程

1.创建ipc对象：

检查对象是否已经存在？如果不存在使用O_CREAT设置了？如果没有返回errno ENOENT，如果设置了，检查系统表格是否全满，满了返回errno ENOENT，否的话成功创建对象	

O_CREAT和O_EXCL都设置了？是返回errno=EEXIST错误

不是的话检查权限是否允许，否的话errno = EACCES  是的话则成功

###2.4IPC权限

新的消息队列、有名信号量或共享内存对象是由其oflag参数中含有O_CREAT标志的mq_open、sem_open、或者shm_open函数创建的。如图2-4所注，权限位与这些IPC类型的每个对象关联，与每个unix文件相互关联。

同样又这三个函数打开一个已经存在的消息队列、信号量或共享内存对象的时候定O_CREAT或制定O_CREAT但是没有制定O_EXCL同事对象已经存在，将基于下面的消息执行权限测试：

1）创建时候赋予这个ipc对象的权限位；
2）锁请求的访问类型（O_RDONLY、O_WRONLY或这O_RDWR）；
3）调用进程的有效用户ID、有效组ID以及各个辅助组ID（若支持的话）.

大多数unix内核按照如下步骤进行权限测试.	

1）如果当前进程的有效用户ID为0（超级用户）,那就允许访问.

2）在当前进程的有效用户ID等于这个ipc对象的属主的前提下，如果相应用户的权限位已经设置，那么就允许访问，否则会拒绝访问.

比如用户以读权限访问ipc的时候，那么这个用户的读权限位必须要设置

3）在当前进程的有效组ID或它的某个辅助组id等于这个ipc对象的组的id前提下，如果响应的组访问权限已经设置，那么就允许访问，否则拒绝.

4）如果相应的其他用户访问权限位已经设置，那么允许访问，否则禁止访问

这4个步骤按照所列的步骤尝试.如果当前的进程应有ipc对象的时候，那么访问权限的授予或者拒绝只依赖与用户访问权限----组访问权限不会考虑.类似的当前进程不应有这个ipc对象，但是它属于某个组，那么访问允许或者拒绝只依赖组，其他用户访问权限绝不会考虑。

###小结：

三种posix ipc------消息队列，共享内存和信号量都是用标识符来标示的.但是这些路径名既可以是文件系统中实际的路径名字，也可以不是，这一点会导致移植性问题书中使用的是px_ipc_name函数.

当我们用open打开ipc对象的时候，我们必须给open的对象访问权限，当打开一个已经存在的ipc对象的时候，锁执行的权限测试与打开一个已经存在的文件时候一样.
