#mysql不可重复读

######什么是mysql的不可重复读

不可重复读 就是一个事务读到另一个事务修改后并提交的数据（update）。在同一个事务中，对于同一组数据读取到的结果不一致。比如，事务B 在 事务A
 提交前读到的结果，和在 事务A 提交后读到的结果可能不同。不可重复读出现的原因就是由于事务并发修改记录而导致的。
 
 ######为什么会出现mysql的不可重复读
 
案例展示:

设置事务的隔离级别
 
```
set session transaction isolation level read committed;
```

依旧是打开两个窗口T1 、 T2并且设置隔离级别

分别开启事务
```
mysql> begin;
Query OK, 0 rows affected (0.00 sec)
```

在T1中插入数据

```
mysql> insert version (id,version) value (10,1);
Query OK, 1 row affected (0.00 sec)

```

T2中查询

```
mysql> select * from version;
+----+---------+
| id | version |
+----+---------+
|  1 |       1 |
|  2 |       1 |
|  3 |       1 |
|  4 |       1 |
|  5 |       1 |
|  6 |       1 |
|  7 |       1 |
|  8 |       1 |
+----+---------+
8 rows in set (0.00 sec)

```

还是没有id为10的数据原因是没提交 在T1中commit

```
mysql> select * from version;
+----+---------+
| id | version |
+----+---------+
|  1 |       1 |
|  2 |       1 |
|  3 |       1 |
|  4 |       1 |
|  5 |       1 |
|  6 |       1 |
|  7 |       1 |
|  8 |       1 |
| 10 |       1 |
+----+---------+
9 rows in set (0.00 sec)

```

在T2中查到了，这明显不符合mysql的隔离性 T2的事务影响到了T1

####如何解决

设置事务的隔离级别 串行化和 可重复读都会避免这个现象的产生.