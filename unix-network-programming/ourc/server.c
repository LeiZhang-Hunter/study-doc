#include "common.h"

void str_echo(int clifd);

void deal_sig_handel(int sig);

int main()
{
	int clifd;
	int sockfd = socket(AF_INET,SOCK_STREAM,0);
	socklen_t len;	

	signal(SIGCHLD,deal_sig_handel);
	
	struct sockaddr_in serversock,clisock;
	bzero(&serversock,sizeof(serversock)); 
	pid_t pid;
	serversock.sin_port = htons(SERVPORT);
	serversock.sin_family = AF_INET;
	serversock.sin_addr.s_addr = inet_addr(LOCAL);
	
	bind(sockfd,(struct sockaddr *)&serversock,sizeof(serversock));

	listen(sockfd,10);

	while(1)
	{
        len = sizeof(clisock);           
		clifd = accept(sockfd,(struct sockaddr *)&clisock,&len);
		printf("%d\n",clifd);
		if(errno == EINTR)
		{
                 printf("EINTR\n",clifd);
			continue;
		}

		if(clifd < 0)
		{
			printf("%d\n",clifd);
			printf("acceept error\n");
			exit(-1);
		}

	       printf("%d connect ok\n",clifd);
		if((pid = fork()) == 0)
		{
			printf("11\n");
			str_echo(clifd);
			printf("close\n");
			close(clifd);
			exit(-1);
						
		}
		close(clifd);
	}	
}

void str_echo(int clifd)
{
	ssize_t n;
	char buf[255];
	
	again:
		while((n=read(clifd,buf,255)) > 0){
        	    printf("read %s\n",buf);
			int result = write(clifd,buf,strlen(buf));
			printf("write %d\n",result);
        }
		if(n < 0&& n == EINTR)
			goto again;
		else if(n<0)
			printf("read %d error\n",clifd);
}

void deal_sig_handel(int sig)
{
	pid_t pid;
	int stat;
			while((pid = waitpid(-1,&stat,WNOHANG)) > 0)
				printf("child %d terminate\n",pid);
}
