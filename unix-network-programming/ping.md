#ping程序实现

common.h

	//
	// Created by zhanglei on 19-2-19.
	//

	#ifndef PING_COMMON_H
	#define PING_COMMON_H

	#endif //PING_COMMON_H

	#include <stdint.h>
	#include <errno.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <netinet/in.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <signal.h>
	#include <unistd.h>
	#include <string.h>
	#include <poll.h>
	#include <limits.h>
	#include <fcntl.h>
	#include <sys/un.h>
	#include <pthread.h>
	#include <netdb.h>
	#define SERVPORT 4000
	#define LOCAL "127.0.0.1"
	#define MAXLINE 255
	#define OPEN_MAX 10
	#define INFTIM -1

	#define max( a, b) \
	( (a) > (b)?(a) : (b) )

	int readline(int fileFd, void* vptr, size_t maxlen)
	{
	    ssize_t n, rc;

	    char c, *ptr;

	    ssize_t result;

	    ptr = vptr;
	    for(n = 1;n<maxlen;n++)
	    {

		again:
		result = read(fileFd,&c,1);
		if(result > 0)
		{
		    *ptr++ = c;
		    if(c == '\n'){
		        break;
		    }
		}

		if(result < 0)
		{
		    if(errno == EINTR)
		        goto again;

		    return -1;
		}else if(result == 0)
		{
		    *ptr = 0;
		    return n-1;
		}


	    }
	    *ptr = 0;
	    return n;
	}


	int writen(int fd, void* vptr, size_t n)
	{
	    size_t nleft;
	    ssize_t nwrite;
	    char* ptr = vptr;
	    nleft = n;

	    if (nleft > 0)
	    {
		if ((nwrite = write(fd, ptr, nleft)) < 0)
		{
		    if (errno = EINTR)
		    {
		        nwrite = 0;
		    }
		    else {
		        return -1;
		    }
		}

		nleft -= nwrite;
		ptr += nwrite;
	    }
	    return n;
	}


	int readn(int fd, void *vptr, size_t n)
	{
	    size_t nleft;
	    ssize_t nread;

	    char* ptr;

	    ptr = vptr;
	    nleft = n;

	    while (nleft > 0)
	    {
		if ((nread = read(fd, ptr, nleft)) <= 0)
		{
		    if (errno = EINTR)
		    {
		        nread = 0;
		    }
		    else if (nread == 0)
		    {
		        break;
		    }
		    else {
		        return -1;
		    }
		}

		nleft = nleft - nread;
		ptr = ptr + nleft;

	    }
	    return nleft;
	}


	void sys_err(const char* str) {
	    printf("%s\n", str);
	    exit(-1);
	}
	
ping.h:

	//
	// Created by zhanglei on 19-2-19.
	//

	#ifndef PING_PING_H
	#define PING_PING_H

	#endif //PING_PING_H

	#include "common.h"
	#include <netinet/in_systm.h>
	#include <netinet/ip.h>
	#include <netinet/ip_icmp.h>

	#define BUFSIZE 1500

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

	struct proto {
	    void (*fproc)(char*,ssize_t,struct msghdr *,struct timeval*);
	    void (*fsend)(void);
	    void(*finit)(void);
	    struct sockaddr *sasend;
	    struct sockaddr *sarecv;

	    socklen_t salen;
	    int icmpproto;
	} *pr;

test.c

	#include "ping.h"
	char* sock_ntop(const struct sockaddr *sa, socklen_t salen)
	{
	    char portstr[8];
	    static char str[128];
	    struct sockaddr_in *sin = (struct sockaddr_in *)sa;
	    if (inet_ntop(AF_INET, &sin->sin_addr, str, sizeof(str)) == NULL)
	    {
		return NULL;
	    }


	    return str;
	}

	struct proto proto_v4 = {proc_v4,send_v4,NULL,NULL,NULL,0,IPPROTO_ICMP};
	int datalen = 56;
	int main(int argc,char** argv)
	{
	    int c;

	    //地址结构体 getaddrinfo的
	    struct addrinfo *ai;

	    //申请一个字符串指针
	    char *h;

	    opterr = 0;

	    while ((c = getopt(argc, argv, "v")) != -1)
	    {
		switch (c)
		{
		    case 'v':
		        verbose++;
		        break;
		    case '?':
		        printf("unknow option:%c",c);
		        exit(-1);
		        break;
		}
	    }

	    if (optind != argc - 1)
		sys_err("usage:ping (-v) <hostname>");

	    //获取主机域名
	    host = argv[optind];

	    //生成进程自己的icmp标志位
	    pid = getpid() & 0xffff;/* ICMP ID field is 16 bits*/

	    signal(SIGALRM,sig_alrm);

	    //获取地址结构体
	    ai = host_serv(host,NULL,0,0);

	    //将地址结构体转化位点进制ip
	    h = sock_ntop(ai->ai_addr,ai->ai_addrlen);

	    printf("host:%s\n", h);

	    printf("PING %s(%s):%d data bytes\n",ai->ai_canonname ? ai->ai_canonname : h,h,datalen);

	    if (ai->ai_family = AF_INET)
	    {
		//初始化结构体
		pr = &proto_v4;
	    }
	    else {
		printf("unknow address family %d",ai->ai_family);
		exit(1);
	    }

	    pr->sasend = ai->ai_addr;
	    pr->sarecv = calloc(1, ai->ai_addrlen);
	    pr->salen = ai->ai_addrlen;


	    readloop();

	    exit(0);
	}

	void readloop(void)
	{
	    int size;
	    char recvbuf[BUFSIZE];
	    char controlbuf[BUFSIZE];

	    struct msghdr msg;
	    struct iovec iov;

	    ssize_t n;
	    struct timeval tval;

	    sockfd = socket(pr->sasend->sa_family,SOCK_RAW,pr->icmpproto);

	    setuid(getuid());

	    if (pr->finit)
	    {
		(*pr->finit)();
	    }

	    size = 60 * 1024;

	    setsockopt(sockfd,SOL_SOCKET,SO_RCVBUF,&size,sizeof(size));

	    sig_alrm(SIGALRM);

	    iov.iov_base = recvbuf;
	    iov.iov_len = sizeof(recvbuf);
	    msg.msg_name = pr->sarecv;
	    msg.msg_iov = &iov;
	    msg.msg_iovlen = 1;
	    msg.msg_control = controlbuf;

	    for (;;) {
		msg.msg_namelen = pr->salen;
		msg.msg_controllen = sizeof(controlbuf);
		n = recvmsg(sockfd,&msg,0);
		if (n < 0)
		{
		    if (errno == EINTR)
		    {
		        continue;
		    }
		    else {
		        sys_err("recvmsg error\n");
		    }
		}
		else if(n == 0){
		    printf("end of file\n");
		    break;
		}
		else {
		    gettimeofday(&tval, NULL);
		    (*pr->fproc)(recvbuf,n,&msg,&tval);
		}
	    }
	}

	void sig_alrm(int signo)
	{
	    (*pr->fsend)();

	    alarm(1);
	    return;
	}

	void send_v4(void)
	{
	    int len;
	    struct icmp *icmp;

	    icmp = (struct icmp *)sendbuf;

	    icmp->icmp_type = ICMP_ECHO;
	    icmp->icmp_code = 0;
	    icmp->icmp_id = pid;//½ø³ÌµÄicmp±êÊ¶
	    icmp->icmp_seq = nsent++;

	    memset(icmp->icmp_data,0xa5,datalen);

	    gettimeofday((struct timeval *)icmp->icmp_data,NULL);

	    len = 8 + datalen;

	    icmp->icmp_cksum = 0;
	    icmp->icmp_cksum = in_cksum((u_short *)icmp,len);

	    sendto(sockfd,sendbuf,len,0,pr->sasend,pr->salen);
	}

	struct addrinfo *host_serv(const char *host, const char *serv, int family, int socktype)
	{
	    int n;
	    struct addrinfo hints, *res;
	    bzero(&hints, sizeof(struct addrinfo));
	    hints.ai_flags = AI_CANONNAME;
	    hints.ai_family = family;
	    hints.ai_socktype = socktype;
	    if ((n = getaddrinfo(host, serv, &hints, &res)) != 0)
	    {
		return NULL;
	    }
	    return res;
	}

	uint16_t in_cksum(uint16_t *addr, int len)
	{
	    int nleft = len;
	    uint32_t sum = 0;
	    uint16_t *w = addr;
	    uint16_t answer = 0;

	    while (nleft > 1)
	    {
		sum += *w++;
		nleft -= 2;
	    }

	    if (nleft == 1) {
		*(unsigned char *)(&answer) = *(unsigned char *)w;
		sum += answer;
	    }

	    sum = (sum >> 16) + (sum & 0xffff);
	    sum += (sum >> 16);
	    answer = ~sum;
	    return answer;
	}

	void proc_v4(char *ptr, ssize_t len, struct msghdr *msg, struct timeval *tvrecv)
	{


	    int hlen1, icmplen;
	    double rtt;
	    struct ip *ip;
	    struct icmp *icmp;
	    struct timeval *tvsend;


	    ip = (struct ip *)ptr;

	    hlen1 = ip->ip_hl << 2;

	    if (ip->ip_p != IPPROTO_ICMP) {
		return;
	    }
	    icmp = (struct icmp *)(ptr + hlen1);

	    if ((icmplen = len - hlen1) < 8)
	    {

		return;
	    }


	    if (icmp->icmp_type == ICMP_ECHOREPLY) {


		if (icmp->icmp_id != pid)
		{
		    printf("error pid:%d\n", icmp->icmp_id);
		    return;
		}

		if (icmplen < 16) {
		    printf("icmplen less len:%d\n", icmplen);
		    return;
		}

		tvsend = (struct timeval *)icmp->icmp_data;


		tv_sub(tvrecv, tvsend);

		rtt = tvrecv->tv_sec * 1000.0 + tvrecv->tv_usec / 1000.0;
		printf(" %d bytes from %s:seq = %u,ttl=%d,rtt=%.3f ms\n", icmplen, sock_ntop(pr->sarecv, pr->salen), icmp->icmp_seq, ip->ip_ttl,rtt);
	    }
	    else if (verbose) {
		printf(" %d bytes from %s:type = %d,code = %d\n", icmplen, sock_ntop(pr->sarecv, pr->salen), icmp->icmp_type, icmp->icmp_code);
	    }
	}

	void tv_sub(struct timeval *out,struct timeval *in)
	{
	    if ((out->tv_usec -= in->tv_usec) < 0) {
		--out->tv_sec;
		out->tv_usec += 1000000;
	    }

	    out->tv_sec -= in->tv_sec;
	}

