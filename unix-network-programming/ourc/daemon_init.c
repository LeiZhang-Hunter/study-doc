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

	//���Թ����ź� ��Ϊ������ͷ���̺��ٴε���fork��Ȼ��exit����εĸ����̺����������ǰ��̨���н��̶�����SIGHUP�ź�,�ٴ�fork��Ŀ����ȷ������ػ������ٴδ�һ������Ҳ�����Զ���ÿ����ն�
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