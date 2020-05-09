#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
int main()
{
    sigset_t set,block_set;
    sigemptyset(&set);
    sigaddset(&set,SIGINT);
    //设置信号屏蔽
    sigfillset(&block_set);
    sigprocmask(SIG_BLOCK,&block_set,NULL);
    int retval;
    for(;;) {
        int res = sigwait(&set, &retval);
        if (res == 0) {
            printf("sigint\n");
        }
    }
    exit(0);
}
