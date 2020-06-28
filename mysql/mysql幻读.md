#mysql幻读

####什么是幻读

除了 SERIALIZABLE 的隔离级别之外都会出现幻读

幻读：指一个线程中的事务读取到了另外一个线程中提交的insert的数据。

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

```
set session transaction isolation level Serializable;
```