#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

void hook(int signo,siginfo_t* info,void* context)
{
    printf("info signo:%d\n",info->si_signo);
    printf("info errno:%d\n",info->si_errno);
    printf("info code:%d\n",info->si_code);
    printf("info pid:%d\n",info->si_pid);
    printf("info uid:%d\n",info->si_uid);
    printf("info band:%ld\n",info->si_band);

    ucontext_t* sig_context = (ucontext_t*)context;//用来保存上下文的
    //常见的ucontext库可以使用

}

int main()
{
    printf("%d\n",getpid());
    struct sigaction action;

    struct sigaction action_info;

    action.sa_flags =  SA_SIGINFO;
    action.sa_handler = (void*)hook;

    sigaction(SIGTERM,&action,NULL);

    while(1){}
}
