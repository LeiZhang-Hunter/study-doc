#include "common.h"

//包含ipv4所需要的头文件
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

#define BUFSIZE 1500

//发送缓冲区
char sendbuf[BUFSIZE];

int datalen;


char *host;
int nsent;
pid_t pid;
int sockfd;
int verbose;

void init_v6(void);
uint16_t in_cksum(uint16_t *addr, int len);
void proc_v4(char*,ssize_t,struct msghdr*,struct timeval *);
void proc_v6(char*, ssize_t, struct msghdr*, struct timeval *);
struct addrinfo *host_serv(const char *host,const char *serv,int family,int socktype);
void send_v4(void);
void send_v6(void);
void readloop(void);
void sig_alrm(int);
void tv_sub(struct timeval*, struct timeval*);

//定义IPV4或者IPV6的头文件
struct proto {
	void (*fproc)(char*,ssize_t,struct msghdr *,struct timeval*);//接收处理函数
	void (*fsend)(void);//发送处理函数
	void(*finit)(void);//初始化的函数
	struct sockaddr *sasend;//发送的地址结构
	struct sockaddr *sarecv;//接收的地址结构

	socklen_t salen;//地址结构体长度
	int icmpproto;
} *pr;