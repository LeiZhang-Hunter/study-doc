#mysql幻读

####什么是幻读

除了 SERIALIZABLE 的隔离级别之外都会出现幻读

幻读：事务A 按照一定条件进行数据读取， 期间事务B 插入了相同搜索条件的新数据，事务A再次按照原先条件进行读取时，发现了事务B 新插入的数据
 称为幻读。

幻读，并不是说两次读取获取的结果集不同，幻读侧重的方面是某一次的 select 操作得到的结果所表征的数据状态无法支撑后续的业务操作。
更为具体一些：select 某记录是否存在，不存在，准备插入此记录，但执行 insert 时发现此记录已存在，无法插入，此时就发生了幻读。

在未提交读（Read uncommitted）和不可重复读（NonRepeatable Read）会出现

####为什么会出现幻读

创建表

```
CREATE TABLE version          (          id INT(11) PRIMARY KEY,          version INT(11)          );
```

我们在T1 T2 两个窗口分别开启事务

T1
```
begin;
```

T2
```
begin
```

T1插入数据并且提交事务

```
mysql> insert version (id,version) value (1,1);
Query OK, 1 row affected (0.01 sec)

mysql> commit;
Query OK, 0 rows affected (0.00 sec)
```

T2里去读

```
mysql> select * from version;
Empty set (0.00 sec)

```

这时候我们在T2里读不到(这时候出现了幻读)

```

mysql> select * from version;
Empty set (0.00 sec)

```

T2里插入数据

```
mysql> insert version (id,version) value (1,1);
ERROR 1062 (23000): Duplicate entry '1' for key 'PRIMARY'
```

######如何解决幻读?

1.设置事务隔离级别为串行化

设置事务隔离级别串行化,全部变为串行了，在另一个插入操作会阻塞，会一直等上一个事务执行完才会执行下一个
```
set session transaction isolation level Serializable;
```

在T1中开启事务后

```
mysql> select * from version;
+----+---------+
| id | version |
+----+---------+
|  1 |       1 |
+----+---------+
1 row in set (0.00 sec)

mysql> insert version (id,version) value (2,1);
Query OK, 1 row affected (0.00 sec)

mysql> commit;
Query OK, 0 rows affected (0.00 sec)

```

在T2中读取

```
mysql> select * from version;
+----+---------+
| id | version |
+----+---------+
|  1 |       1 |
|  2 |       1 |
+----+---------+
2 rows in set (0.00 sec)
```

没有幻读了


2.select.....for update ....RR级别下防止幻读

如果 id = 1 的记录存在则会被加行（X）锁，如果不存在，则会加 next-lock key / gap 锁（范围行锁），即记录存在与否，mysql 都会对记录应该
对应的索引加锁，其他事务是无法再获得做操作的。

这里我们就展示下 id = 1 的记录不存在的场景，FOR UPDATE 也会对此 “记录” 加锁，要明白，InnoDB 的行锁（gap锁是范围行锁，一样的）锁定的是
记录所对应的索引，且聚簇索引同记录是直接关系在一起的。