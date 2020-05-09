#include "common.h"

int daemon_proc;

int daemon_init()
{
	int i;
	pid_t pid;

	if ((pid = fork()) < 0)
	{
		return -1;
	}else if (pid)
	{
		exit(-1);
	}

	if (setsid() < 0)
	{
		return -1;
	}

	//忽略挂起信号 因为创建领头进程后再次调用fork，然后exit掉这次的父进程后会向进程组的前后台所有进程都发送SIGHUP信号,再次fork的目的是确保这个守护进程再次打开一个进程也不会自动获得控制终端
	signal(SIGHUP, SIG_IGN);

	if ((pid = fork()) < 0)
	{
		return -1;
	}
	else if (pid)
	{
		exit(-1);
	}

	daemon_proc = 1;

	chdir("/");

	for (i = 0; i < 64; i++)
		close(i);

	open("/dev/null",O_RDONLY);
	open("/dev/null", O_RDWR);
	open("/dev/null", O_RDWR);

	return 0;
}

int main(int argc, char** argv)
{
	daemon_init();

	while (1)
	{
		
	}
}