system消息队列

####6.5 msgctl函数

msgctl提供了3个命令

```
#include <sys/msg.h>
int msgctl(int msqid,int cmd,struct msqid_ds *buff);
```

IPC_RMID 从系统中删除由msqid指定的消息队列。当前在该队列上的任何消息都会被丢弃，那么msgctl第三个参数将会被丢弃

IPC_SET 给指定 的消息队列设置msqid_ds结构一下4个成员：msg_perm.uid、msg_perm.gid、msg_perm.mode和msg_gbytes。他们的值由buff参数指向的结构中的相应成员。

IPC_STAT给调用者返回所指定的消息队列对应的当前msqid_ds结构。

下面我们来写一个简单的例子：

```

****
//
// Created by root on 19-1-6.
//
#include "web.h"
#include "unpifi.h"
#include <sys/msg.h>
#define MSG_R 0400 /* read permission */
#define MSG_W 0200 /* write permission */
#define SVMSG_MODE (MSG_R | MSG_W | MSG_R >>3 | MSG_R >>6)

struct msgbuf{
    long type;
    char mtext[1];
};
int main(int argc,char** argv)
{
    int msqid;

    struct msqid_ds info;
    struct msgbuf buf;

    msqid = msgget(IPC_PRIVATE,SVMSG_MODE | IPC_CREAT);

    buf.type = 1;

    buf.mtext[0] = 1;

    msgsnd(msqid,&buf,1,0);

    msgctl(msqid,IPC_STAT,&info);

    printf("read-write:%03o,cbytes = %lu,qnum = %lu,qbytes = %lu\n",info.msg_perm.mode & 0777,
            (__syscall_ulong_t)info.__msg_cbytes,(__syscall_ulong_t)info.msg_qnum,(__syscall_ulong_t)info.msg_qbytes);

    printf("ipcs -q");

    msgctl(msqid,IPC_RMID,NULL);
    exit(0);
}

```
