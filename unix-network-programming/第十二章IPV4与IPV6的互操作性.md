 #12章  IPV4与IPV6的互操作性

####12.1概述

IPV4有一天会被用完，所以我们写程序的时候要注意IPV4向IPV6程序的移植。

####12.2 IPV4客户与ipv6服务器

双栈主机的特性是既可以处理IPV4 又可以处理IPV6。是通过IPV4映射的IPV6实现的。

当accept系统用把接受的ipv6客户连接返回给服务器进程的时候，该ipv6套接字就是ipv6首部中的源地址，没有做任何改动。这个连接上其余的数据报都是ipv6数据报。

1）ipv6服务器启动后创建一个ipv6的监听套接字，我们假定服务器把通配地址绑定到该套接字上。

2）ipv4客户可以通过gethostbyname 获取到一个A记录，但是服务器主机既有一个A记录也有一个AAAA记录，因为它及支持ipv4有支持ipv6，不过ipv4客户需要的是一个A记录。

3）客户调用connect 发送一个ipv4 SYN到服务器主机

4）服务器主机接受到目的地址为ipv6监听套接字ipv4的syn，设置一个标志指示本连接应使用ipv4映射的ipv6地址，然后响应一个ipv4 SYN/ACK，建立连接后，返回服务器一个ipv4映射的ipv6地址


5）当服务器主机往这个ipv4映射的ipv6地址发送TCP分节的时候，其ip栈产生的目的地址为所映射ipv4地址的ipv4载送数据报。因此客户和服务器所有数据通讯都采用ipv4

6）除非服务器显示检查这个ipv6地址是不是一个ipv4映射的ipv6地址，否则我们永远不知道他是一个ipv4地址。同样这个ipv4的客户机也不知道他自己在与一个ipv6通讯

同样udp 请求也是如此.

大多数双栈主机在处理套接字上使用以下规则：

1）ipv4套接字监听只能监听ipv4地址

2）如果一个ipv6套接字绑定了通配符的ip地址，但是又没有设置IPV6_V6ONLY那么这个套接字既可以接受ipv6地址，又可以接受ipv4地址

3）如果服务器有一个ipv6监听套接字地址，而且绑定了ipv4映射的ipv6地址之外的某个非通配地址，而且海设置了IPV6_V6ONLY套接字选项，那么只能接受ipv6客户的外来连接。

####12.3 IPV6客户与IPV4服务器

1）一个IPV4服务器只支持在IPV4主机上启动后创建一个IPV4监听套接字

2）IPV6客户启动以后调用getaddrinfo查找IPV6，hints结构体中设置了AI_V4MAPPED,既然只支持IPV4那么服务器主机上只有A记录

3）IPV6套接字地址结构中设置IPV4映射的IPV6地址后调用connect，发送一个SYN到服务器

4）服务器响应一个IPV4的SYN，终于通过IPV4数据报建立    

注意无论UDP与TCP ipv4都不可以指定ipv6

####12.4 ipv6地址测试宏

	#include <netinet/in.h>
	int IN6_IS_ADDR_UNSPECIFIED(const struct in6_addr *aptr);
	int IN6_IS_ADDR_LOOPBACK(const struct in6_addr *aptr);
	int IN6_IS_ADDR_MULTICAST(const struct in6_addr *aptr);
	int IN6_IS_ADDR_LINKLOCAL(const struct in6_addr *aptr);
	int IN6_IS_ADDR_SITELOCAL(const struct in6_addr *aptr);
	int IN6_IS_ADDR_V4MAPPED(const struct in6_addr *aptr);
	int IN6_IS_ADDR_V4COMPAT(const struct in6_addr *aptr);

	int IN6_IS_ADDR_MC_NODELOCAL(const struct in6_addr *aptr);
	int IN6_IS_ADDR_MC_LINKLOCAL(const struct in6_addr *aptr);
	int IN6_IS_ADDR_MC_SITELOCAL(const struct in6_addr *aptr);
	int IN6_IS_ADDR_MC_ORGLOCAL(const struct in6_addr *aptr);
	int IN6_IS_ADDR_MC_GLOBAL(const struct in6_addr *aptr);

前7个宏测试ipv6的基本类型。后5个宏测试ipv6的多播地址范围。

ipv6客户可以调用IN6_IS_ADDR_V4MAPPED宏测试由解析器返回的ipv6地址。ipv6服务器同样可以调用这个宏来测试accept或者recvfrom返回的ipv6地址。

作为需要使用这个宏的一个例子，让我们来考虑ftp和它的port指令。如果启动一个ftp的客户端让它连接到ftp服务器，然后发出ftp的dir指令，那么ftp客户就像ftp服务器发送一个port指令，这条指令会把ftp客户的ip地址和服务器告诉服务器，然后就此建立数据连接。然而ipv6的FTP客户必须清楚对端是一个ipv4服务器还是一个ipv6服务器，因为两者port指令格式不同。前者需要的格式“PORT a1,a2,a3,a4,P1,P2”,其中前者四个数字，构成一个4字节的ipv4地址，后两个数字构成端口号。后者需要一个ERPT指令，包含一个地址簇、文本格式地址、和文本格式端口号。