#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
static void sig_quit(int);
int main()
{
    sigset_t newmask,oldmask,pendmask;
    if(signal(SIGQUIT,sig_quit) == SIG_ERR)
    {
        printf("can not cactch SIGQUIT\n");
        exit(-1);
    }
    sigemptyset(&newmask);
    sigaddset(&newmask,SIGQUIT);
    if(sigprocmask(SIG_BLOCK,&newmask,&oldmask) < 0)
    {
        printf("SIG BLOCK ERROR\n");
        exit(-1);
    }
    sleep(5);
    if(sigpending(&pendmask) < 0)
    {
        printf("sigpending error\n");
        exit(-1);
    }
    if(sigismember(&pendmask,SIGQUIT))
    {
        printf("\nSIGQUIT pending\n");
    }
    if(sigprocmask(SIG_SETMASK,&oldmask,NULL) < 0)
    {
        printf("SIGQUIT setmask error\n");
        exit(-1);
    }
    printf("SIGQUIT unblocked\n");

    sleep(5);
    exit(0);
}
static void sig_quit(int signo)
{
    printf("caught SIGQUIT\n");
    if(signal(SIGQUIT,SIG_DFL) == SIG_ERR)
    {
        printf("can't reset SIGQUIT\n");
        exit(-1);
    }
}