#mysql的锁

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

