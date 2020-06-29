#mysql的事务日志redo-log和undo-log

redo-log 是重做日志 提供的是前滚操作

undo-log 是回滚日志 提供回滚操作

redo-log是常用的物理日志，记录的是数据页的物理修改，不是一行或者某几行修改成怎么样，它用来恢复提交后的物理数据页。

undo是用来回滚记录到某个版本的。undo log是逻辑日志，根据每行记录进行记录。

####redo-log

redo log不是二进制日志

1.redo log是innodb层产生的，只记录该存储引擎中表的修改。并且二进制日志先于redo log被记录

2.redo-log记录的是物理页的修改，redo-log同一个事务可能会记录多次，redo-log是并发写入的

redo-log有两部分：

1）一部分是日志缓冲区

2）另一部分是磁盘日志，这个日志是持久的

MySQL支持用户自定义在commit时如何将log buffer中的日志刷log file中。这种控制通过变量 innodb_flush_log_at_trx_commit 的值来决定。
该变量有3种值：0、1、2，默认为1。但注意，这个变量只是控制commit动作是否刷新log buffer到磁盘。

当设置为1的时候，事务每次提交都会将log buffer中的日志写入os buffer并调用fsync()刷到log file on disk中。这种方式即使系统崩溃也不会丢失任何数据，但是因为每次提交都写入磁盘，IO的性能较差。
当设置为0的时候，事务提交时不会将log buffer中日志写入到os buffer，而是每秒写入os buffer并调用fsync()写入到log file on disk中。也就是说设置为0时是(大约)每秒刷新写入到磁盘中的，当系统崩溃，会丢失1秒钟的数据。
当设置为2的时候，每次提交都仅写入到os buffer，然后是每秒调用fsync()将os buffer中的日志写入到log file on disk。

redo-log 的存储日志块

可以将结构规整为

块 头 -----块体--------块尾

块头记录了块的一些信息：

```
 log_block_hdr_no：(4字节)该日志块在redo log buffer中的位置ID。
 log_block_hdr_data_len：(2字节)该log block中已记录的log大小。写满该log block时为0x200，表示512字节。
 log_block_first_rec_group：(2字节)该log block中第一个log的开始偏移位置。
 lock_block_checkpoint_no：(4字节)写入检查点信息的位置。
```

如果产生的日质量超过一个块，会用多个块做记录

日志尾只有一个部分： log_block_trl_no ，该值和块头的 log_block_hdr_no 相等。


1.4 log group和redo log file

查看log的部分信息

```
mysql> show global variables like "innodb_log%";
+-----------------------------+----------+
| Variable_name               | Value    |
+-----------------------------+----------+
| innodb_log_buffer_size      | 16777216 |
| innodb_log_checksums        | ON       |
| innodb_log_compressed_pages | ON       |
| innodb_log_file_size        | 50331648 |
| innodb_log_files_in_group   | 2        |
| innodb_log_group_home_dir   | ./       |
| innodb_log_write_ahead_size | 8192     |
+-----------------------------+----------+
7 rows in set (0.00 sec)

```

可以看到在默认的数据目录下，有两个ib_logfile开头的文件，它们就是log group中的redo log file，而且它们的大小完全一致且等于变量 
innodb_log_file_size 定义的值。第一个文件ibdata1是在没有开启 innodb_file_per_table 时的共享表空间文件，对应于开启 innodb_file_per_table 时的.ibd文件。

在innodb将log buffer中的redo log block刷到这些log file中时，会以追加写入的方式循环轮训写入。即先在第一个log file（即ib_logfile0）
的尾部追加写，直到满了之后向第二个log file（即ib_logfile1）写。当第二个log file满了会清空一部分第一个log file继续写入。

由于是将log buffer中的日志刷到log file，所以在log file中记录日志的方式也是log block的方式。

在每个组的第一个redo log file中，前2KB记录4个特定的部分，从2KB之后才开始记录log block。除了第一个redo log file中会记录，log group中
的其他log file不会记录这2KB，但是却会腾出这2KB的空间。如下：

1.6 日志刷盘的规则

log buffer中未刷到磁盘的日志称为脏日志(dirty log)。

刷日志到磁盘有以下几种规则：

1.发出commit动作时。已经说明过，commit发出后是否刷日志由变量 innodb_flush_log_at_trx_commit 控制。

2.每秒刷一次。这个刷日志的频率由变量 innodb_flush_log_at_timeout 值决定，默认是1秒。要注意，这个刷日志频率和commit动作无关。

3.当log buffer中已经使用的内存超过一半时。

4.当有checkpoint时，checkpoint在一定程度上代表了刷到磁盘时日志所处的LSN位置。


1.9 innodb的恢复行为

在启动innodb的时候，不管上次是正常关闭还是异常关闭，总是会进行恢复操作。

因为redo log记录的是数据页的物理变化，因此恢复的时候速度比逻辑日志(如二进制日志)要快很多。而且，innodb自身也做了一定程度的优化，让恢复速度变得更快。

重启innodb时，checkpoint表示已经完整刷到磁盘上data page上的LSN，因此恢复时仅需要恢复从checkpoint开始的日志部分。例如，当数据库在上一次
checkpoint的LSN为10000时宕机，且事务是已经提交过的状态。启动数据库时会检查磁盘中数据页的LSN，如果数据页的LSN小于日志中的LSN，则会从检查
点开始恢复。


####2.undo log


undo log有两个作用：提供回滚和多个行版本控制(MVCC)。
另外，undo log也会产生redo log，因为undo log也要实现持久性保护。

在数据修改的时候，不仅记录了redo，还记录了相对应的undo，如果因为某些原因导致事务失败或回滚了，可以借助该undo进行回滚。

undo log和redo log记录物理日志不一样，它是逻辑日志。可以认为当delete一条记录时，undo log中会记录一条对应的insert记录，反之亦然，
当update一条记录时，它记录一条对应相反的update记录。

当执行rollback时，就可以从undo log中的逻辑记录读取到相应的内容并进行回滚。有时候应用到行版本控制的时候，也是通过undo log来实现的：当读
取的某一行被其他事务锁定时，它可以从undo log中分析出该行记录以前的数据是什么，从而提供该行版本信息，让用户实现非锁定一致性读取。

undo log是采用段(segment)的方式来记录的，每个undo操作在记录的时候占用一个undo log segment。


2.2 undo log的存储方式

rollback segment称为回滚段，每个回滚段中有1024个undo log segment。

在以前老版本，只支持1个rollback segment，这样就只能记录1024个undo log segment。后来MySQL5.5可以支持128个rollback segment，即支持
128*1024个undo操作，还可以通过变量 innodb_undo_logs (5.6版本以前该变量是 innodb_rollback_segments )自定义多少个rollback segment，默认值为128。

```
ls: 无法打开目录'/var/lib/mysql': 权限不够
zhanglei@zhanglei-OptiPlex-9020:~$ sudo ls /var/lib/mysql
auto.cnf    ca.pem	     client-key.pem   eolinker_os     ibdata1	   ib_logfile1	monitor  mysql_upgrade_info  private_key.pem  pureliving	  school	   server-key.pem	   sys
ca-key.pem  client-cert.pem  debian-5.7.flag  ib_buffer_pool  ib_logfile0  ibtmp1	mysql	 performance_schema  public_key.pem   pureliving_monitor  server-cert.pem  sff_20190505_wordpress
```

ibdata1 就是undolog的日志


2.3 和undo log相关的变量

```
 mysql> show variables like "%undo%";
+-------------------------+-------+
| Variable_name           | Value |
+-------------------------+-------+
| innodb_undo_directory   | .     |
| innodb_undo_logs        | 128   |
| innodb_undo_tablespaces | 0     |
+-------------------------+-------+
```


```
当事务提交的时候，innodb不会立即删除undo log，因为后续还可能会用到undo log，如隔离级别为repeatable read时，事务读取的都是开启事务时的
最新提交行版本，只要该事务不结束，该行版本就不能删除，即undo log不能删除。

但是在事务提交的时候，会将该事务对应的undo log放入到删除列表中，未来通过purge来删除。并且提交事务时，还会判断undo log分配的页是否可以重用，
如果可以重用，则会分配给后面来的事务，避免为每个独立的事务分配独立的undo log页而浪费存储空间和性能。

通过undo log记录delete和update操作的结果发现：(insert操作无需分析，就是插入行而已)

delete操作实际上不会直接删除，而是将delete对象打上delete flag，标记为删除，最终的删除操作是purge线程完成的。
update分为两种情况：update的列是否是主键列。
如果不是主键列，在undo log中直接反向记录是如何update的。即update是直接进行的。
如果是主键列，update分两部执行：先删除该行，再插入一行目标行。
```

