/*
 *����˫��ͨ��
 */
#include<stdio.h>  
#include<string.h>  
#include<sys/types.h>  
#include<stdlib.h>  
#include<unistd.h>  
#include<sys/socket.h>  


int main()
{
	int sv[2];    //һ���������׽���������  
	if (socketpair(AF_LOCAL, SOCK_STREAM, 0, sv) < 0)   //�ɹ������� ʧ�ܷ���-1  
	{
		perror("socketpair");
		return 0;
	}

	pid_t id = fork();        //fork���ӽ���  
	if (id == 0)               //����  
	{
		//close(sv[0]); //���ӽ����йرն�  
		close(sv[1]); //���ӽ����йرն�  

		const char* msg = "i am children\n";
		char buf[1024];
		while (1)
		{
			// write(sv[1],msg,strlen(msg));  
			write(sv[0], msg, strlen(msg));
			sleep(1);

			//ssize_t _s = read(sv[1],buf,sizeof(buf)-1);  
			ssize_t _s = read(sv[0], buf, sizeof(buf) - 1);
			if (_s > 0)
			{
				buf[_s] = '\0';
				printf("children say : %s\n", buf);
			}
		}
	}
	else   //����  
	{
		//close(sv[1]);//�ر�д�˿�  
		close(sv[0]);//�ر�д�˿�  
		const char* msg = "i am father\n";
		char buf[1024];
		while (1)
		{
			//ssize_t _s = read(sv[0],buf,sizeof(buf)-1);  
			ssize_t _s = read(sv[1], buf, sizeof(buf) - 1);
			if (_s > 0)
			{
				buf[_s] = '\0';
				printf("father say : %s\n", buf);
				sleep(1);
			}
			// write(sv[0],msg,strlen(msg));  
			write(sv[1], msg, strlen(msg));
		}
	}
	return 0;
}