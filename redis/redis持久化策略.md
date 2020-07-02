#RDB（redis database）

redis 持久化流程

既然redis的数据可以保存在磁盘上，那这个流程是什么样子的呢？

要有下面这5个流程

1）客户端向服务端发送指令，比如说写

2）服务端收到写请求，写入到内存中

3）服务端调用write继续写入磁盘

4）操作系统将缓冲区中的数据转移到磁盘控制器上(数据在磁盘缓存中)。

5）磁盘控制器将数据写到磁盘的物理介质中。

redis 出现故障4,5都是可以执行的 如果说机器崩了 那么4,5不可执行。

####1.RDB机制

把数据用快照的形式存储到磁盘。RDB持久化是指在指定的时间间隔将内存数据写入磁盘。默认文件名字是dump.rdb

默认配置在redis.conf中

提供了3种机制：save、bgsave、自动化。

1.save触发方式:

1)save：这里是用来配置触发 Redis的 RDB 持久化条件，也就是什么时候将内存中的数据保存到硬盘。比如“save m n”。表示m秒内数据集存在n次修改时
，自动触发bgsave。

默认如下配置：

表示900 秒内如果至少有 1 个 key 的值变化，则保存save 900 1#表示300 秒内如果至少有 10 个 key 的值变化，则保存save 300 10#表示60 秒内
如果至少有 10000 个 key 的值变化，则保存save 60 10000

不需要持久化，那么你可以注释掉所有的 save 行来停用保存功能。

具体见redis.conf 里的配置

```
################################ SNAPSHOTTING  ################################
#
# Save the DB on disk:
#
#   save <seconds> <changes>
#
#   Will save the DB if both the given number of seconds and the given
#   number of write operations against the DB occurred.
#
#   In the example below the behaviour will be to save:
#   after 900 sec (15 min) if at least 1 key changed
#   after 300 sec (5 min) if at least 10 keys changed
#   after 60 sec if at least 10000 keys changed
#
#   Note: you can disable saving completely by commenting out all "save" lines.
#
#   It is also possible to remove all the previously configured save
#   points by adding a save directive with a single empty string argument
#   like in the following example:
#
#   save ""

save 900 1
save 300 10
save 60 10000
```

2)stop-writes-on-bgsave-error:默认值为yes。当启用了RDB且最后一次后台保存数据失败，Redis是否停止接收数据。这会让用户意识到数据没有正确
持久化到磁盘上，否则没有人会注意到灾难（disaster）发生了。如果Redis重启了，那么又可以重新开始接收数据了

```
# By default Redis will stop accepting writes if RDB snapshots are enabled
# (at least one save point) and the latest background save failed.
# This will make the user aware (in a hard way) that data is not persisting
# on disk properly, otherwise chances are that no one will notice and some
# disaster will happen.
#
# If the background saving process will start working again Redis will
# automatically allow writes again.
#
# However if you have setup your proper monitoring of the Redis server
# and persistence, you may want to disable this feature so that Redis will
# continue to work as usual even if there are problems with disk,
# permissions, and so forth.
stop-writes-on-bgsave-error yes
```

3)rdbcompression ；默认值是yes。对于存储到磁盘中的快照，可以设置是否进行压缩存储。

```
# Compress string objects using LZF when dump .rdb databases?
# For default that's set to 'yes' as it's almost always a win.
# If you want to save some CPU in the saving child set it to 'no' but
# the dataset will likely be bigger if you have compressible values or keys.
rdbcompression yes
```

4)rdbchecksum

```
默认值是yes。在存储快照后，我们还可以让redis使用CRC64算法来进行数据校验，但是这样做会增加大约10%的性能消耗，如果希望获取到最大的性能提升
，可以关闭此功能。
```

5)dbfilename:设置快照的文件名，默认是 dump.rdb
```
# The filename where to dump the DB
dbfilename dump.rdb
```

6)dir：设置快照文件的存放路径，这个配置项一定是个目录，而不能是文件名。
```
# The working directory.
#
# The DB will be written inside this directory, with the filename specified
# above using the 'dbfilename' configuration directive.
#
# The Append Only File will also be created inside this directory.
#
# Note that you must specify a directory here, not a file name.
dir /var/lib/redis
```

####4.RDB的优势和劣势

```
①、优势

（1）RDB文件紧凑，全量备份，非常适合用于进行备份和灾难恢复。

（2）生成RDB文件的时候，redis主进程会fork()一个子进程来处理所有保存工作，主进程不需要进行任何磁盘IO操作。

（3）RDB 在恢复大数据集时的速度比 AOF 的恢复速度要快。
```

劣势：

```
RDB快照是一次全量备份，存储的是内存数据的二进制序列化形式，存储上非常紧凑。当进行快照持久化时，会开启一个子进程专门负责快照持久化，
子进程会拥有父进程的内存数据，父进程修改内存子进程不会反应出来，所以在快照持久化期间修改的数据不会被保存，可能丢失数据。

```

#AOF机制

####1.持久化原理(Append Only File)