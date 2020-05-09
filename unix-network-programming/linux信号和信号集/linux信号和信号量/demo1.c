#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/acct.h>

void hook(int signo)
{
    printf("stop\n");
}

int main(int argc,char** argv)
{
    printf("%d\n",getpid());

    signal(SIGTERM,hook);

    while(1)
    {

    }
    exit(0);
}
