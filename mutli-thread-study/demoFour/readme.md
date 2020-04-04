# 内容介绍

这篇笔记学习自陈硕先生的 c++ 多线程服务器编程


上一节中bind 传入 this指针，暗藏风险，因为StockFactory::get() 把原始指针 this 保存到了 boost::function，如 果 StockFactory 的
 生 命 期 比 Stock 短, 那 么 Stock 析 构 时 去 回 调 StockFactory::deleteStock 就会 core dump。因为你无法知道这个指针是否存活
 ，所以我们可以enable_shared_from_this，所以我们使用boost::enable_shared_from_this<StockFactory>,把this指针变成shared_ptr传
 进去，当然申请方式由之前的

```
StockFactory factory;
```

变为了

```
shared_ptr<StockFactory> factory(new StockFactory);
```