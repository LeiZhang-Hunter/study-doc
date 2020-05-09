#include <signal.h>
#include <stdio.h>
#include <unistd.h>
void sigintctl(int signum)

{   printf( "handle begin.\n");
    printf( "receive signum %d \n" , signum) ;
    printf( "handle end.\n");
}
int main()
{
    pid_t pid = getpid();
//     signal( SIGINT, sigintctl) ;
    signal( SIGRTMIN, sigintctl) ;
    kill(-1,SIGRTMIN);
    return 0;
}