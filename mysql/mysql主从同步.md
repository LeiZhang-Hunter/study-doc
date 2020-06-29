#Mysql主从同步原理简介

学习地址:

```
https://www.cnblogs.com/shengguorui/p/11355921.html
```

1.定义：当master(主)库的数据发生变化的时候，变化会实时的同步到slave(从)库。

2.好处：

1）水平扩展数据库的负载能力。

2）容错，高可用。Failover(失败切换)/High Availability

3）数据备份。


3.实现：在master机器上，主从同步事件会被写到特殊的log文件中(binary-log);在slave机器上，slave读取主从同步事件，并根据读取的事件变化，
在slave库上做相应的更改。

4.主从同步事件：

在master机器上，主从同步事件会被写到特殊的log文件中(binary-log);

主从同步事件有3种形式:statement、row、mixed。

1）statement：会将对数据库操作的sql语句写入到binlog中。

2）row：会将每一条数据的变化写入到binlog中。

3）mixed：statement与row的混合。Mysql决定什么时候写statement格式的，什么时候写row格式的binlog。


在master机器上的操作

当master上的数据发生改变的时候，该事件(insert、update、delete)变化会按照顺序写入到binlog中。

binlog dump线程

当slave连接到master的时候，master机器会为slave开启binlog dump线程。当master 的 binlog发生变化的时候，binlog dump线程会通知slave，
并将相应的binlog内容发送给slave。

在slave机器上的操作

当主从同步开启的时候，slave上会创建2个线程。

I/O线程。该线程连接到master机器，master机器上的binlog dump线程会将binlog的内容发送给该I/O线程。该I/O线程接收到binlog内容后，再将内容写入到本地的relay log。
SQL线程。该线程读取I/O线程写入的relay log。并且根据relay log的内容对slave数据库做相应的操作。




