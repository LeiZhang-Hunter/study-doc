#include "ping.h"
//此函数将sockaddr结构体转换为ip地址仅仅局限于ipv4
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

//初始化proto的ipv4的地址结构
struct proto proto_v4 = {proc_v4,send_v4,NULL,NULL,NULL,0,IPPROTO_ICMP};

/*
*可选数据长度，把随同回射请求发送的可选数据量设置为56个字节，由此产生了84个字节的ipv4数据报（20个字节的ip首部以及ICMP首部）或者106个字节的ipv6数据报
*随同某个回射请求发送的任何数据必须在对应的回射应答中返送回来。我们将在这个数据的前8个字节存放本回射请求发送时刻的时间戳，然后在收到对应的回射应答之时使用返送回来的时间戳来计算RTT
*/
int datalen = 56;
int main(int argc,char** argv)
{
	int c;
	struct addrinfo *ai;
	char *h;

	opterr = 0;

	//处理命令行
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

	host = argv[optind];

	//ICMP的进程标识
	pid = getpid() & 0xffff;/* ICMP ID field is 16 bits*/

	//创建闹钟处理函数，一秒钟触发一次
	signal(SIGALRM,sig_alrm);

	ai = host_serv(host,NULL,0,0);

	h = sock_ntop(ai->ai_addr,ai->ai_addrlen);

	printf("host:%s\n", h);

	printf("PING %s(%s):%d data bytes\n",ai->ai_canonname ? ai->ai_canonname : h,h,datalen);

	//初始化全局结构体
	if (ai->ai_family = AF_INET)
	{
		pr = &proto_v4;
	}
	else {
		printf("unknow address family %d",ai->ai_family);
		exit(1);
	}

	//初始化发送的结构体
	pr->sasend = ai->ai_addr;

	//初始化接收的结构体
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
	icmp->icmp_id = pid;//进程的icmp标识
	icmp->icmp_seq = nsent++;

	//先在该ICMP消息的数据部分填充以值为0xa5的模式，再在这个数据部分的开始存入当前时间
	memset(icmp->icmp_data,0xa5,datalen);

	gettimeofday((struct timeval *)icmp->icmp_data,NULL);

	len = 8 + datalen;

	icmp->icmp_cksum = 0;	
	//为了计算ICMP校验和，我们先把校验和字段设置为0，再调用in_cksum函数，并把返回值存入校验和字段。ICMPV4的校验和字段计算覆盖了ICMPV4首部后跟的任何数据
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