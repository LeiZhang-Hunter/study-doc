#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/acct.h>
#include <pwd.h>
#include <string.h>

static void my_alarm(int signo)
{
    struct passwd *rootptr;
    printf("in signal handle\n");

    if((rootptr = getpwnam("root")) == NULL)
    {
        printf("getpwname(root) error \n");
        exit(-1);
    }

    alarm(1);
}

int main(int argc,char** argv)
{
    struct passwd *ptr;

    signal(SIGALRM,my_alarm);

    for(;;)
    {
        if((ptr == getpwnam("sar")) == NULL)
        {
            printf("getpwnam error\n");
            exit(-1);
        }

        if(strcmp(ptr->pw_name,"sar") != 0)
        {
            printf("return value corrupted!pw name = %s\n"
                    ,ptr->pw_name);
            exit(-1);
        }
    }
}
