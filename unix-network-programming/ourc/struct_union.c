#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h> 
typedef struct{
	union
	{
		struct sockaddr_in inet_v4;
		struct sockaddr_in6 inet_v6;
		struct sockaddr_un un;
	}addr;
	socklen_t len;

}swSocketAddress;

int main()
{
	swSocketAddress a;
	a.len = 11;
	printf("%d\n",a.len);
	return 0;
}
