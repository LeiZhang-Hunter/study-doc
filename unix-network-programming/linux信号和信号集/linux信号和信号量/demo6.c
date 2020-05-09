#include <stdio.h>
#include <signal.h>
#include <unistd.h>
void term_hook(int signo)
{
    printf("stop\n");
}
int main()
{
    printf("%d\n",getpid());
    signal(SIGTERM,term_hook);
    sigset_t set;
    sigset_t old_set;
    sigfillset(&set);
    sigprocmask(SIG_BLOCK,&set,&old_set);
    //sigprocmask(SIG_UNBLOCK,&set,&old_set);
    //sigdelset(&set,SIGTERM);
    //sigprocmask(SIG_SETMASK,&set,&old_set);
    while(1)
    {

    }
    return 0;
}
