#include <signal.h>
#include <stdio.h>
#include <unistd.h>
void sigintctl(int signum)

{   printf( "handle begin.\n");
    printf( "receive signum %d \n" , signum) ;
    sleep(2);
    printf( "handle end.\n");
}
int main()
{
    printf("%d\n",getpid());
//     signal( SIGINT, sigintctl) ;
    signal( SIGRTMIN, sigintctl) ;
    while(getchar() != "q"){};
    return 0;
}