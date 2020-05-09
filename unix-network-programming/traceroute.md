#traceroute程序

1.概念 

首先我们要知道什么是traceroute，traceroute (Windows 系统下是tracert) 命令利用ICMP 协议定位您的计算机和目标计算机之间的所有路由器。TTL 值可以反映数据包经过的路由器或网关的数量，通过操纵独立ICMP 呼叫报文的TTL 值和观察该报文被抛弃的返回信息，traceroute命令能够遍历到数据包传输路径上的所有路由器.

TTL 就是 time to live  表示一个数据包从出发开始一共可以被中转多少次（路由器和服务器）

我们自己开发一个traceroute程序。与之前的ping 一样，traceroute允许我们确定ip数据报从本地主机游历到某个远程主机所经过的路径，他的操作比较简单.traceroute使用ipv4的TTL字段或ipv6的跳限字段以及两种icmp消息.这个数据报导致第一跳路由器返送一个ICMP的time exceeded in transmit 传输超时的错误.接着每次递增TTL一次发送一个UDP数据包，从而逐步确定下一跳路由器.当某个UDP数据报到达最终的目的地时候，目标是由这个主机返送一个ICMP “port unreachable”（端口不可达错误）.这个目标通过向一个随机选取的未被目的主机使用的端口发送UDP数据报得以实现.

早期的traceroute 通过IP_HDRINCL套接字选项套接字直接构造自己的ipv4首部来设置TTL字段.然而如今的系统提供了IP_TTL字段，他允许我们指定外出数据的TTL.设置这个套接字选项比构造完整的ipv4首部容易的多.ipV6的IPV6_YBICAST_HOPS套接字选项允许我们控制IPV6数据报的跳限字段.