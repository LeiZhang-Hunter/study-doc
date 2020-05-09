#include <stdio.h>
#include <signal.h>
#include <unistd.h>
void alarm_hook(int signo)
{
    printf("alarm\n");
}
int main()
{
    signal(SIGALRM,alarm_hook);
    alarm(1);
    pause();
    return 0;
}
