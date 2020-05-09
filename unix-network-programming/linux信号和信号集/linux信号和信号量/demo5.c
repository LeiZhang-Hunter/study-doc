#include <stdio.h>
#include <signal.h>

int main()
{
    int i = 0;
    //树的跟
    sigset_t set;
    sigfillset(&set);
    for(i=1;i<=64;i++)
    {
        printf("%d\n",sigismember(&set,i));
    }
    sigdelset(&set,15);
    printf("%d\n",sigismember(&set,15));
    sigaddset(&set,15);
    printf("%d\n",sigismember(&set,15));
    sigemptyset(&set);
    printf("%d\n",sigismember(&set,15));
    printf("%d\n",sigismember(&set,14));
    return 0;
}
