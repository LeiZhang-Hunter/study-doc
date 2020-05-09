#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>

int run = 1;
void function(int signo)
{
    run = 0;
}
int main()
{
    printf("%d\n",getpid());
    signal(SIGTERM,function);
    fd_set set;
    FD_ZERO(&set);
    int fd = socket(AF_INET,SOCK_STREAM,0);
    FD_SET(fd,&set);
    struct sockaddr_in addr;
    bind(fd,(struct sockaddr*)&addr,sizeof(struct sockaddr));
    listen(fd,100);
    while (run)
    {
        int res = select(fd+1,&set,NULL,NULL,NULL);
        if(res < 0)
        {
            printf("%d:%s\n",errno,strerror(errno));
        }
    }
    printf("code end!thank you!\n");
}
