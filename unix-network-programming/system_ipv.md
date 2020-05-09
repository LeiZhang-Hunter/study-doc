#system ipv c

system v ipv消息队列：

system v消息队列

system v 信号量

system v 共享内存

消息队列 头文件：

<sys/msg.h>

创建或者打开消息队列：msgget

控制ipc函数：msgctl

ipc操作函数：msgsnd
			msgrcv
			
			
信号量：

<sys/sem.h>

创建或者打开ipc的函数：

	semget
	
控制ipc函数：

	semctl
	
ipc操作函数：
	
	semop

共享内存区：

	<sys/shm.h>

创建或打开ipc函数：

	shmget
	
控制ipc函数

	shmctl
	
ipc控制函数

	shmat
	shmdt
	
	
####3.2 key_t键和ftok函数

头文件<sys/ipc.h>

	#include <sys/ipc.h>
	
	key_t ftok(const char *pathname,int id);
	
这个函数把从pathnamme到处的信息与id的低序8位组合成一个整数ipc键

该函数嘉定对于使用system v ipc某个给定的应用来说，客户和服务器同意使用对该应用有一定意义的pathname。他可以是服务器守护程序的路径名、服务器使用的某个公共数据文件的路径名或者系统上的某个其他路径名。如果客户和服务器之间需要单个ipc通道，那么可以使用譬如说值为1的id。需要多个ipc通道，从服务器到客户又一个通道，那么作为一个例子，一个通道可使用值为1的id，另一个通道使用值为2的id。服务和客户程序一旦达成一致，双方就可以使用ftok函数把pathbane额id转换成同一个ipc键。

ftok的实现调用stat函数，然后组合以下的三个值。

1）pathname锁在的文件系统信息

2）这个文件在本文件系统内的索引节点号

3）ud的低序8位

如果path不存在胡返回-1

	#include <unistd.h>
	#include <stdio.h>
	#include <net/if.h>
	#include <stdlib.h>
	#include <ucontext.h>
	#include <stdbool.h>
	#include <sys/ipc.h>
	int main(int argc, const char *argv[]){
	    printf("key:%x\n",ftok("/test1/prifinfo.c",0x57));
	    return 0;
	}

	key:5701002a
	
	
####3.3 ipc_perm结构

	struct ipc_perm{
		uid_t uid;
		gid_t gid;
		uid_t cuid;
		gid_t cgid;
		mode_t mode;
		ulong_t seq;
		key_t key;
	};
	
	
####3.4 创建与打开ipc通道

创建或打开一个ipc对象的三个get函数的第一个参数key是类型为key_t的ipc的键，返回值identifier是一个整数标识符。该表示服不同于ftok函数的id参数，我们不就会看到。对于key的值由两个选择。

1）调用ftok，给他传递pathname和id

2）指定key为IPC_PRIVATE,浙江保证会创建一个新的唯一的ipc对象。


####3.5 IPC权限

使用某个get函数，指定IPC_CRET函数创建一个新的ipc函数对象时候，以下信息就保存到该对象的ipc_perm结构中。


####3.6 标识符重用


ipc_perm有一个漕位使用情况序列号，他是一个ipc对象的计数器	，当删除一个ipc对象的维护计数器，每当删除一个ipc对象，内核就递增相应的漕位号，若溢出则循环回0.

描述符是属于特定进程的，system ipc消息队列是系统范围的不属于特定进程。

msgget  semget  shmget 也获取一个ipc标识符这些函数返回也属于整数。不过他们意义适用于所有进程，如果是无学院关系的进程，也可以使用单个消息队列，那么由msgget函数返回的该消息队列的表示服在双方进程中必须是同一个整数值，这样双方才会访问同一个消息队列。


	#include <unistd.h>
	#include <stdio.h>
	#include <net/if.h>
	#include <stdlib.h>
	#include <ucontext.h>
	#include <stdbool.h>
	#include <sys/ipc.h>
	#include <sys/msg.h>
	#include <sys/types.h>
	int main(int argc, const char *argv[]){
	    int i,msqid;

	    for(i=0;i<50;i++)
	    {
		msqid = msgget(IPC_PRIVATE,MSG_MORE | IPC_CREAT);

		printf("msqid=%d\n",msqid);

		msgctl(msqid,IPC_RMID,NULL);
	    }
	    return 0;
	}


####3.7 ipcs和ipcrm函数

ipcs输出有关system v ipc特性的各种信息，ipcrm则删除一个system v的消息队列、信号量或共享内存区。


####3.8 内核限制

system v ipc的多数实现有内在的内核限制，例如消息队列的最大数目、每个信号量采集的最大信号量数等等。

内核限制往往太小。我们可以修改这些限制

可以通过修改/etc/system文件语句来修改

sysconfig -q ipc可以查看限制。