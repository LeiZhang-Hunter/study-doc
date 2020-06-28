#mysql的锁

##mysql行锁

####mysql 的读锁

mysql的读锁，也叫共享锁

如何使用?

```
select * from table where ... LOCK IN SHARE MODE
```

两个窗口操作同一个数据可以读但是不可写

T1 窗口 和 T2 窗口

T1中使用

```
select * from version where id=1 lock in share mode;
``` 

T2中使用

```
 update version set version=44 where id=1;
```

发生了阻塞，时间超过之后变为了

```
mysql> update version set version=44 where id=1;
ERROR 1205 (HY000): Lock wait timeout exceeded; try restarting transaction
```

但是不会阻塞

```
mysql> select * from version where id=1 lock in share mode;
+----+---------+
| id | version |
+----+---------+
|  1 |       1 |
+----+---------+
1 row in set (0.00 sec)

mysql> 

```

####mysql 写锁

也叫排他锁

就是我们常用的select * from table where ..... for update

一个写锁会阻塞其他写操作和其他的锁，注意这时候不会影响其他的读操作
开启事务后t1 
```
begin
```

后加入排他锁

```
mysql> select * from version where id=1 for update;
+----+---------+
| id | version |
+----+---------+
|  1 |       1 |
+----+---------+
1 row in set (0.00 sec)
```

窗口t2
更新操作阻塞住了
```
mysql> update version set version=44 where id=1;
```

加入共享锁

```
mysql> select * from version where id=1 lock in share mode;

```
阻塞住了

######乐观锁

上面说的读写锁都是行级锁，下面说一下乐观锁

上述介绍的是行级锁，可以最大程度地支持并发处理（同时也带来了最大的锁开销）乐观锁是一种逻辑锁，通过数据的版本号（vesion）的机制来实现，
极大降低了数据库的性能开销。

我们为表添加一个字段 version，读取数据时将此版本号一同读出，之后更新时，对此版本号+1，同时将提交数据的version 与数据库中对应记录的当前
version 进行比对，如果提交的数据版本号大于数据库表当前版本号，则予以更新，否则认为是过期数据

乐观锁在并发量比较大的时候推荐使用 比如说这时候来了多个并发查询

```
mysql> select * from version where id=1;
+----+---------+
| id | version |
+----+---------+
|  1 |       1 |
+----+---------+
1 row in set (0.00 sec)

```

都拿到了版本号是1的一行数据这时候多个线程或者进程进行更新

```
UPDATE version 
SET data=10000, version=version+1 
WHERE id=1 and version=1
```

这时候只有一个语句会更新成功

这有点像一个非阻塞模式，但是这个失败之后不会重试 而其他的悲观锁会穿行，等待上一个完成之后继续执行，要根据业务场景酌情使用。

####意向共享锁 和意向排他锁

意向共享锁，简称IS，其作用在于：通知数据库接下来需要施加什么锁并对表加锁。如果需要对记录A加共享锁，那么此时innodb会先找到这张表，对该表加意
向共享锁之后，再对记录A添加共享锁。

意向排他锁，简称IX，其作用在于：通知数据库接下来需要施加什么锁并对表加锁。如果需要对记录A加排他锁，那么此时innodb会先找到这张表，对该表加意
向排他锁之后，再对记录A添加排他锁。

意向共享锁和意向排它锁是数据库主动加的，不需要我们手动处理

####间隙锁
间隙锁是封锁索引记录中的间隔，或者第一条索引记录之前的范围，又或者最后一条索引记录之后的范围。

产生间隙锁的条件（RR事务隔离级别下；）：

使用普通索引锁定；
使用多列唯一索引；
使用唯一索引锁定多行记录。

以上情况，都会产生间隙锁，下面是小编看了官方文档理解的：

对于使用唯一索引来搜索并给某一行记录加锁的语句，不会产生间隙锁。（这不包括搜索条件仅包括多列唯一索引的一些列的情况；在这种情况下，会产生间
隙锁。）例如，如果id列具有唯一索引，则下面的语句仅对具有id值100的行使用记录锁，并不会产生间隙锁：
SELECT * FROM child WHERE id = 100 FOR UPDATE;
这条语句，就只会产生记录锁，不会产生间隙锁。


##mysql的表锁

1.MyISAM 不支持事务会锁表
MySQL的存储引擎是从MyISAM到InnoDB，锁从表锁到行锁。后者的出现从某种程度上是弥补前者的不足。比如：MyISAM不支持事务，InnoDB支持事务。
表锁虽然开销小，锁表快，但高并发下性能低。行锁虽然开销大，锁表慢，但高并发下相比之下性能更高。事务和行锁都是在确保数据准确的基础上提高并发
的处理能力。本章重点介绍InnoDB的行锁。

2.没有命中索引行锁会自动升级为表锁
当我们使用排它锁进行查询，但这时候检索条件没有命中索引，这时候会触发表锁，这个表不能进行任何的写入或者更新

T1表 查询version 这时候version 并没有加索引，这样就不会命中索引
```
mysql> begin;
Query OK, 0 rows affected (0.00 sec)

mysql> select * from version where version=44 for update;
+----+---------+
| id | version |
+----+---------+
|  1 |      44 |
+----+---------+
1 row in set (0.00 sec)

```

这时候会触发表锁,在t2中进行写入或者更新都会阻塞住，不能对这个表进行任何操作

```
mysql> insert version (id,version) value (14,1);
^C^C -- query aborted
ERROR 1317 (70100): Query execution was interrupted
mysql> update version set version=444 where id=10;
^C^C -- query aborted
ERROR 1317 (70100): Query execution was interrupted
mysql> select * from version where id=1;
+----+---------+
| id | version |
+----+---------+
|  1 |      44 |
+----+---------+
1 row in set (0.00 sec)

```

只是可以进行读取

结论：行锁没有命中索引的时候会升级为表锁。

原因是没加锁的时候mysql要扫描表，这时候mysql认为加很多行锁没有必要，会自动升级为表锁。


