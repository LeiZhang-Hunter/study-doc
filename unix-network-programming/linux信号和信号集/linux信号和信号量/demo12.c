#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
volatile sig_atomic_t quitflag;
static void sig_int(int);
void err_sys(const char* msg)
{
    printf("%s\n",msg);
    exit(-1);
}
int main()
{
    sigset_t newmask,oldmask,zeromask;
    if(signal(SIGINT,sig_int) == SIG_ERR)
        err_sys("signal SIGINT error");
    if(signal(SIGQUIT,sig_int) == SIG_ERR)
        err_sys("signal SIGINT error");
    sigemptyset(&zeromask);
    sigemptyset(&newmask);
    sigaddset(&newmask,SIGQUIT);
    if(sigprocmask(SIG_BLOCK,&newmask,&oldmask)<0)
        err_sys("SIGBLOCK error");
    while (quitflag == 0)
        sigsuspend(&zeromask);
    quitflag = 0;
    if(sigprocmask(SIG_SETMASK,&oldmask,NULL)<0)
        err_sys("SIG_SETMASK error");
    exit(0);
}
static void sig_int(int signo)
{
    if(signo== SIGINT)
        printf("SIGINT\n");
    else if(signo == SIGQUIT)
        quitflag = 1;
}