####scull设计

https://www.jianshu.com/p/487a12a06dbe

scull 简称simple character utility for loading localities 区域装载的简单字符串工具

scull0 ~ scull3 
这四个设备分别由一个全局而且持久的内存区域组成。全局是指：如果设备被多次打开，则打开它的所有文件描述符共享该设备所包含的数据。持久是指：如果设备关闭后再打开，其中的数据不会丢失
可以使用常用命令来访问和测试这个设备，如cp、cat以及shell的io重定向等。

scullpipe0~scullpipe3

这四个FIFO设备和管道类似，一个进程读由另一个进程写入的数据。如果多个进程读取同一个设备，他们就会发生数据竞争。虽然实际的驱动程序让硬件中断和它们的设备保持同步，但是阻塞模式和非阻塞
模式操作是同一个内容，并且有别与中断处理。

scullsingle
scullpriv
sculluid
scullwuid

scullsingle一次只允许一个进程使用这个内核驱动程序，而scullpriv对每个虚拟控制台都是私有的，这是因为每个控制台终端的进程将获取不同的内存区。
sculluid和scullwuid可以被多次打开，但每次只能被一个用户打开，如果另一个用户也打开就会显示Device Busy，scullwuid则实现了阻塞方式的open


####主备号和次设备号

字符串设备的访问是通过系统内部的设备名称进行的。这些特殊文件位于/dev下面，可以通过
    ls -l /dev
    
查看开头是c的表示字符串设备，块设备也在/dev下面由b来标识

用ls -l /dev查看下面文件

    crw-rw----  1 root tty       7,    65 11月  1 11:43 vcsu1
    crw-rw----  1 root tty       7,    66 11月  1 11:43 vcsu2
    crw-rw----  1 root tty       7,    67 11月  1 11:43 vcsu3
    crw-rw----  1 root tty       7,    68 11月  1 11:43 vcsu4
    crw-rw----  1 root tty       7,    69 11月  1 11:43 vcsu5
    crw-rw----  1 root tty       7,    70 11月  1 11:43 vcsu6
    drwxr-xr-x  2 root root            60 11月  1 11:43 vfio
    crw-------  1 root root     10,    63 11月  1 11:43 vga_arbiter
    crw-------  1 root root     10,   137 11月  1 11:43 vhci
    crw-------  1 root root     10,   238 11月  1 11:43 vhost-net
    crw-------  1 root root     10,   241 11月  1 11:43 vhost-vsock
    crw-rw----+ 1 root video    81,     0 11月  1 11:43 video0
    crw-rw----+ 1 root video    81,     1 11月  1 11:43 video1
    drwxr-xr-x  2 root root            60 11月  1 11:43 wmi
    crw-rw-rw-  1 root root      1,     5 11月  1 11:43 zero
    crw-------  1 root root     10,   249 11月  1 11:43 zfs

在修改日期前面是主设备号和此设备号，现在linux系统中，允许多个设备共享主设备号，但是大多数设备扔然按照一个主设备号对应一个驱动设备的原则

设备编号的内部表达式

dev_t 用来保存设备编号，在linux内核2.6中，dev_t是一个32位的数字，12位用来表示主设备号，20位用来表示次设备号，我们的代码不应该对设备编号的组织做任何假定，应该使用
<linux/kdev_t.h>中定义的宏。

主设备号应该使用

    MAJOR(dev_t dev)
    MINOR(dev_t dev)
    
####分配和释放设备编号

注册设备编号

int register_chrdev_region(dev_t first,unsigned int count, char* name);

我们经常不需要知道使用哪些主设备，因此linux 内核社区一直在努力转向linux设备编号的动态分配，运行下面的函数内核将会分配主设备编号

int alloc_chrdev_region(char* dev,unsigned int firstminor, unsigned int count,char*name);

firstminor是使用请求的第一个次设备编号，它通常是0。count和name参与register_chrdev_region函数是一样的。

设备编号释放：

void unregister_chrdev_region(dev_t first,unsigned int count);

####动态分配主设备号

作为驱动程序者，我们有下面这些选择：可以简单选一个尚未使用的编号，或者通过动态方式分配主设备编号。如果使用程序只有我们自己，则选定一个编号的方法永远行得通；然而一旦
驱动程序广泛使用，随机选定的主设备号可能造成冲突和麻烦。

所以驱动程序应该使用alloc_chrdev_region,而不是使用register_chrdev_region

动态分配的缺点：由于分配的主设备号始终不能保持一致，所以无法预先创建设备节点。一旦分配了设备号，就可以从/proc/devices中读取的到。

因此，为了加载一个使用动态主设备编号程序，对insmod可替换为一个动态的脚本，这个脚本在调用insmod后读取/proc/devices获得新分配的设备号，然后创建对应的设备文件。

典型的/proc/devices文件如下：

字符串设备如下：

    1.mem
    2.pty
    3.ttyp
    4.ttys
    6.lp
    7.vcs
    21.sg
    180.usb

等等

块设备

    2.fd
    8.sd
    11.sr
    65.sd
    66.sd
    
####scull的注册

```c
struct scull_dev {
    struct scull_gset* data;//指向第一个量子集的指针
    int quantum; //当前量子的大小
    int qset; //当前数组的大小
    unsigened long size; //保存其中的数据量
    unsigened int access_key;//由sculluid和scullpriv使用
    struct semaphore sem;//互斥信号量
    struct cdev cdev;//字符串设备结构
}
```

scull 初始化字符串结构体如下


####open和release

open里应该完成下面的这些工作：

- 检查设备特定的错误
- 如果设备是首次打开，对其进行初始化
- 如有必要，更新f_op指针
- 分配并填写放置与filp—>private_data里的数据结构

首先要确定要打开的设备。open方法如下