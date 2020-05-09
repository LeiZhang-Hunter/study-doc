#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <ucontext.h>
#include <setjmp.h>

jmp_buf context_buffer;


void hook(int signo)
{
    printf("stop\n");

    siglongjmp(context_buffer,1);
}


int main()
{
    printf("%d\n",getpid());
    signal(SIGTERM,hook);

//    sigsetjmp(context_buffer,0);
    sigsetjmp(context_buffer,1);
    while(1)
    {

    }
}
