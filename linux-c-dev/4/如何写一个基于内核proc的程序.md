#linux内核5.8中proc编程的学习

对着proc_fs.h的头文件，一点点学习

看到头文件里第一个结构体，/proc 目录的操作结构体

```c
struct proc_ops {
        unsigned int proc_flags;
        int     (*proc_open)(struct inode *, struct file *);
        ssize_t (*proc_read)(struct file *, char __user *, size_t, loff_t *);
        ssize_t (*proc_write)(struct file *, const char __user *, size_t, loff_t *);
        loff_t  (*proc_lseek)(struct file *, loff_t, int);
        int     (*proc_release)(struct inode *, struct file *);
        __poll_t (*proc_poll)(struct file *, struct poll_table_struct *);
        long    (*proc_ioctl)(struct file *, unsigned int, unsigned long);
#ifdef CONFIG_COMPAT
        long    (*proc_compat_ioctl)(struct file *, unsigned int, unsigned long);
#endif
        int     (*proc_mmap)(struct file *, struct vm_area_struct *);
        unsigned long (*proc_get_unmapped_area)(struct file *, unsigned long, unsigned long, unsigned long, unsigned long);
} __randomize_layout;

```

再看头文件中重要的函数

```c
extern struct proc_dir_entry *proc_create_data(const char *, umode_t,
                                               struct proc_dir_entry *,
                                               const struct proc_ops *,
                                               void *);
```

好了写下第一个demo

```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h> /* copy_*_user */
#include <linux/slab.h>

int our_proc_open(struct inode *, struct file *);
ssize_t our_proc_read(struct file *, char __user *, size_t, loff_t *);

struct proc_ops ops = {
        .proc_open = our_proc_open,
        .proc_read = our_proc_read
};

int our_proc_open(struct inode *inode, struct file *file)
{
    return 0;
};

ssize_t our_proc_read(struct file *file, char __user * buffer, size_t size, loff_t *off_t)
{
    printk(KERN_ALERT "HELLO,our_proc_read\n");
    return 0;
};

static int __init our_proc_init(void)
{
    printk(KERN_ALERT "HELLO,KERNEL\n");

    //挂载proc
    proc_create_data("our_proc", 0644, NULL, &ops, NULL);
    return 0;
}

static void __exit our_proc_exit(void)
{
    printk(KERN_ALERT "HELLO,KERNEL BYE BYE\n");
    remove_proc_entry("our_proc", NULL);
}

module_init(our_proc_init);
module_exit(our_proc_exit);

```

Makefile

```
obj-m:=proc.o
        KERNELBUILD := /lib/modules/$(shell uname -r)/build
default:
	        make -C $(KERNELBUILD) M=$(shell pwd) modules
clean:
	        rm -rf *.o *.ko
```

执行命令

```
sudo make
sudo insmod proc.ko
sudo rmmod proc.ko
```

好了整个proc目录崩了

```
ls /proc
```

电脑整个命令行崩溃，点击关机按钮，机器卡死。。。。。。

这是什么原因呢？引起了我的思考

vim prc_fs.h的头文件，我想我找到了原因

```c
#define remove_proc_entry(name, parent) do {} while (0)
```

这个函数被注释掉了，也就是说不再起作用了，然后在源码中找到了proc_remove 这个函数
但是看实现好像不会有作用，因为实现代码中什么也没有
```
static inline void proc_remove(struct proc_dir_entry *de) {}
```

然后再次修正demo

```
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h> /* copy_*_user */
#include <linux/slab.h>

#define MODULE_NAME "our_proc"

struct proc_dir_entry* proc_handle = NULL;

int our_proc_open(struct inode *, struct file *);
ssize_t our_proc_read(struct file *, char __user *, size_t, loff_t *);

struct proc_ops ops = {
        .proc_open = our_proc_open,
        .proc_read = our_proc_read
};

int our_proc_open(struct inode *inode, struct file *file)
{
    return 0;
};

ssize_t our_proc_read(struct file *file, char __user * buffer, size_t size, loff_t *off_t)
{
    printk(KERN_ALERT "HELLO,our_proc_read\n");
    return 0;
};

static int __init our_proc_init(void)
{
    printk(KERN_ALERT "HELLO,KERNEL\n");

    //挂载proc
    proc_handle = proc_create_data(MODULE_NAME, 0644, NULL, &ops, NULL);
    return 0;
}

static void __exit our_proc_exit(void)
{
    printk(KERN_ALERT "HELLO,KERNEL BYE BYE\n");
    if (proc_handle) {
        proc_remove(proc_handle);
    }
}

module_init(our_proc_init);
module_exit(our_proc_exit);

```

再次执行命令

```
sudo make
sudo insmod proc.ko
cat /proc/our_proc
dmesg
```

在调试的环形缓冲区中看到了

```
[ 1212.214871] HELLO,KERNEL
[ 1227.271966] HELLO,our_proc_read
[ 1250.484078] HELLO,KERNEL BYE BYE
[ 1305.898772] HELLO,KERNEL
[ 1342.722686] HELLO,KERNEL BYE BYE
[ 1649.965351] HELLO,KERNEL
[ 1656.079254] HELLO,our_proc_read
[ 1657.011422] HELLO,our_proc_read
```

卸载驱动

```
sudo rmmod proc.ko
```

看到/proc/our_proc没了，一切恢复了正常

这就是proc新的函数api，主要是用来动态查询驱动信息的，当然我们调用
glibc 经典的api read write ioctl lseek等等都会触发响应的钩子