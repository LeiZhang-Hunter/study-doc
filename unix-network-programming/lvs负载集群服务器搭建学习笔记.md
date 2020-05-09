#lvs负载集群服务器搭建学习笔记

##概述

lvs是负载调度器，我们的客户机上不需要安装任何软件，但是调度机器上必须要安装lvs的核心软件是ipvs和实现软件ipvsadm，而真实的服务器不需要安装软件 

####1.检查Load Balancer服务器是否已支持ipvs。

	modprobe -l|grep ipvs

返回的结果为：

	modprobe: invalid option -- 'l'

所以我们输入 
	modprobe --help 查看所有的返回结果

返回结果

	Usage:
		modprobe [options] [-i] [-b] modulename
		modprobe [options] -a [-i] [-b] modulename [modulename...]
		modprobe [options] -r [-i] modulename
		modprobe [options] -r -a [-i] modulename [modulename...]
		modprobe [options] -c
		modprobe [options] --dump-modversions filename
	Management Options:
		-a, --all                   Consider every non-argument to
		                            be a module name to be inserted
		                            or removed (-r)
		-r, --remove                Remove modules instead of inserting
		    --remove-dependencies   Also remove modules depending on it
		-R, --resolve-alias         Only lookup and print alias and exit
		    --first-time            Fail if module already inserted or removed
		-i, --ignore-install        Ignore install commands
		-i, --ignore-remove         Ignore remove commands
		-b, --use-blacklist         Apply blacklist to resolved alias.
		-f, --force                 Force module insertion or removal.
		                            implies --force-modversions and
		                            --force-vermagic
		    --force-modversion      Ignore module's version
		    --force-vermagic        Ignore module's version magic
	
	Query Options:
		-D, --show-depends          Only print module dependencies and exit
		-c, --showconfig            Print out known configuration and exit
		-c, --show-config           Same as --showconfig
		    --show-modversions      Dump module symbol version and exit
		    --dump-modversions      Same as --show-modversions
	
	General Options:
		-n, --dry-run               Do not execute operations, just print out
		-n, --show                  Same as --dry-run
		-C, --config=FILE           Use FILE instead of default search paths
		-d, --dirname=DIR           Use DIR as filesystem root for /lib/modules
		-S, --set-version=VERSION   Use VERSION instead of `uname -r`
		-s, --syslog                print to syslog, not stderr
		-q, --quiet                 disable messages
		-v, --verbose               enables more messages
		-V, --version               show version
		-h, --help                  show this help

不懂的地方解读：

modprobe是linux的一个命令，可载入指定的个别模块，或是载入一组相依的模块。

-a 将每一个非参数视为要删除或插入的模块名。

-r 删除模块

-R --resolve-alias  仅仅查看和打印别名然后退出。 --first-time  如果模块已经插入或者移除则失败

-f 强迫操作

上面的命令失效，我们可以使用下面的命令检查lvs模块

	find /lib/modules/$(uname -r)/ -iname "**.ko*" | cut -d/ -f5-

####2.安装依赖包

Kernel-devel、gcc、openssl、openssl-devel、popt 、popt-devel、libnl、libnl-devel、popt-static


	169  yum install kernel-devel
	170  yum install gcc
	171  yum install openssl-devel
	172  yum install popt
	173  yum install popt-devel
	174  yum install libnl
	175  yum install libnl-devel
	176  yum install popt-static

####3.在load balancer  服务器上安装ipvsadm

1）在安装ipvsadm之前，先使用以下命令查看当前的负载服务器的服务器内核版本

	rpm -q kernel-devel

输出结果：

	kernel-devel-3.10.0-957.1.3.el7.x86_64

下载软件
 
	wget http://www.linuxvirtualserver.org/software/kernel-2.6/ipvsadm-1.26.tar.gz

解压包

	tar zxvf ipvsadm-1.26.tar.gz

建立需要编译的软连接

	ln -s /usr/src/kernels/3.10.0-957.1.3.el7.x86_64/ /usr/src/linux

编译并安装ipvsadm。

	make && make install

输入命令ipvsadm检查是否安装成功

	[root@localhost ~]# ipvsadm
	IP Virtual Server version 1.2.1 (size=4096)
	Prot LocalAddress:Port Scheduler Flags
	  -> RemoteAddress:Port           Forward Weight ActiveConn InActConn

####4.集群部署

lvs提供了三种调度模式：

1）使用VS/NAT调度

首先我们需要考虑在什么情况下可以使用NAT方式来搭建负载集群

nat本身是一种将私有地址转化为合法地址的一种技术，在vs/nat内，整个集群只有一个合法的外部地址，这个ip在load balancer 上对外可见，其他的真实服务器与这个调度器组成了一个对外不可见的内网，internet上的用户访问集群必须通过对外提供ip地址的调度器，在利用nat技术，将真实服务器置于网络上。

形象一些比喻，在vs/Nat模式下，调度器相当于网络三层模型的路由器（这个比喻十分不恰当，但是十分便于理解），路由器有一个wlan口，wan口提供一个在互联网上唯一的且合法的ip地址，同样路由器也有lan口，与服务器的真实服务器连接，真实的网管指向路由器的lan口地址。当用户访问路由器的时候，路由器会更改数据包包头的目的地址为某个为真实的服务器仲的地址，然后交给真实的服务器处理，当真实的服务器处理完毕之后，会根据数据包的源地址返还给用户，但是由于真实服务器地址采用路由器ip为网关，所以真实服务器的数据包必须经过路由器，路由器接收到数据包之后，将数据包报头的源地址修改为wan口地址，然后再发送给用户。这种数据传输方式是vs/nat传输方式。
