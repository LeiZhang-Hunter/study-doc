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

    longjmp(context_buffer,1);
}


int main()
{
    printf("%d\n",getpid());
    signal(SIGTERM,hook);

    setjmp(context_buffer);

    while(1)
    {

    }
}
