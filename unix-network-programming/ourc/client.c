#include "common.h"

void str_cli(int clifd);

int main()
{
	int clifd;
	socklen_t len;
	clifd = socket(AF_INET,SOCK_STREAM,0);

	struct sockaddr_in cliaddr;
	bzero(&cliaddr,sizeof(cliaddr));
	cliaddr.sin_family = AF_INET;
	cliaddr.sin_port = htons(SERVPORT);
	cliaddr.sin_addr.s_addr = INADDR_ANY;


	int result = connect(clifd,(struct sockaddr *)&cliaddr,sizeof(cliaddr));
	if(result < 0)
	{
		printf("connect error\n");
		exit(-1);
	}
	str_cli(clifd);
	exit(-1);
}


//str_cliµÚ¶þ°æ
void str_cli(int clifd)
{
	fd_set rset;
	FD_ZERO(&rset);
	int max_number;
	int recv_fd_number;
	char sendline[255],recvline[255];
	for(;;)
	{
		FD_SET(clifd,&rset);
		FD_SET(fileno(stdin),&rset);
		max_number = max(clifd,fileno(stdin))+1;
		recv_fd_number = select(max_number,&rset,NULL,NULL,NULL);
		if(recv_fd_number > 0)
		{
			if(FD_ISSET(clifd,&rset))
			{
				if(readline(clifd,recvline,255) == 0)
				{
					printf("server error\n");
					exit(-1);
				}else{
					printf("recv content:%s\n",&recvline);
					//fputs(recvline,stdout);
				}
				
			}

			if(FD_ISSET(fileno(stdin),&rset))
			{
				if(fgets(sendline,255,stdin) == NULL)
				{
					printf("stdout recv error\n");
					exit(-1);
				}else{
					printf("stdint recv :%s\n",&sendline);
					int write_result = writen(clifd,sendline,strlen(sendline));
				}
			}
		}else{
			if(errno == EINTR)
			{
				continue;
			}else{
				printf("select error,errcode:%d\n",errno);
				exit(-1);
			}
		}
	}
}




