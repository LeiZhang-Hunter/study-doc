#mysql脏读

####mysql的脏读 幻读 更新丢失 以及不可重复读

mysql事务的特性

原子性 一致性 隔离性 持久性

隔离性是每一个事务不受影响，不会因为A事务的修改或写入操作影响b事务

同时隔离性又有4个隔离级别

read uncommitted、read committed、repeatable red、serialize

如果不考虑事务的隔离性会出现下面的问题

######脏读是什么

一个事务可以读取到另一个线程未提交的事务

隔离级别在 未提交读（Read uncommitted） 时候可能出现

######为什么会出现脏读
案例分析
我们首先设置事务为未隔离级别 设置为 未提交读

```
set session transaction isolation level read uncommitted;
```

这时候我们去开启事务修改一张表的数据

```
begin;

update enterprise set state=2 where id=15;

select * from enterprise where id=15\G;

```

这时候查询的state 是 2

我们用另一个窗口去查询数据 也是 2，这时候没有提交数据这时候窗口值应该是1的 出现了脏读

```
select * from enterprise where id=15\G;
```

这时候突然发生回滚

```
rollback
```

这时候就会出现了脏读

######怎么解决脏读

将事务级别设置为repeatable red 这是mysql默认的设置级别

```
set session transaction isolation level Repeatable Read;
```

如何查看隔离级别:

```
select @@tx_isolation
```