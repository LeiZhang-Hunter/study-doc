#include "ping.h"
//�˺�����sockaddr�ṹ��ת��Ϊip��ַ����������ipv4
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

//��ʼ��proto��ipv4�ĵ�ַ�ṹ
struct proto proto_v4 = {proc_v4,send_v4,NULL,NULL,NULL,0,IPPROTO_ICMP};

/*
*��ѡ���ݳ��ȣ�����ͬ���������͵Ŀ�ѡ����������Ϊ56���ֽڣ��ɴ˲�����84���ֽڵ�ipv4���ݱ���20���ֽڵ�ip�ײ��Լ�ICMP�ײ�������106���ֽڵ�ipv6���ݱ�
*��ͬĳ�����������͵��κ����ݱ����ڶ�Ӧ�Ļ���Ӧ���з��ͻ��������ǽ���������ݵ�ǰ8���ֽڴ�ű�����������ʱ�̵�ʱ�����Ȼ�����յ���Ӧ�Ļ���Ӧ��֮ʱʹ�÷��ͻ�����ʱ���������RTT
*/
int datalen = 56;
int main(int argc,char** argv)
{
	int c;
	struct addrinfo *ai;
	char *h;

	opterr = 0;

	//����������
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

	//ICMP�Ľ��̱�ʶ
	pid = getpid() & 0xffff;/* ICMP ID field is 16 bits*/

	//�������Ӵ�������һ���Ӵ���һ��
	signal(SIGALRM,sig_alrm);

	ai = host_serv(host,NULL,0,0);

	h = sock_ntop(ai->ai_addr,ai->ai_addrlen);

	printf("host:%s\n", h);

	printf("PING %s(%s):%d data bytes\n",ai->ai_canonname ? ai->ai_canonname : h,h,datalen);

	//��ʼ��ȫ�ֽṹ��
	if (ai->ai_family = AF_INET)
	{
		pr = &proto_v4;
	}
	else {
		printf("unknow address family %d",ai->ai_family);
		exit(1);
	}

	//��ʼ�����͵Ľṹ��
	pr->sasend = ai->ai_addr;

	//��ʼ�����յĽṹ��
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
	icmp->icmp_id = pid;//���̵�icmp��ʶ
	icmp->icmp_seq = nsent++;

	//���ڸ�ICMP��Ϣ�����ݲ��������ֵΪ0xa5��ģʽ������������ݲ��ֵĿ�ʼ���뵱ǰʱ��
	memset(icmp->icmp_data,0xa5,datalen);

	gettimeofday((struct timeval *)icmp->icmp_data,NULL);

	len = 8 + datalen;

	icmp->icmp_cksum = 0;	
	//Ϊ�˼���ICMPУ��ͣ������Ȱ�У����ֶ�����Ϊ0���ٵ���in_cksum���������ѷ���ֵ����У����ֶΡ�ICMPV4��У����ֶμ��㸲����ICMPV4�ײ�������κ�����
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