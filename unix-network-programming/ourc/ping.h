#include "common.h"

//����ipv4����Ҫ��ͷ�ļ�
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

#define BUFSIZE 1500

//���ͻ�����
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

//����IPV4����IPV6��ͷ�ļ�
struct proto {
	void (*fproc)(char*,ssize_t,struct msghdr *,struct timeval*);//���մ�����
	void (*fsend)(void);//���ʹ�����
	void(*finit)(void);//��ʼ���ĺ���
	struct sockaddr *sasend;//���͵ĵ�ַ�ṹ
	struct sockaddr *sarecv;//���յĵ�ַ�ṹ

	socklen_t salen;//��ַ�ṹ�峤��
	int icmpproto;
} *pr;