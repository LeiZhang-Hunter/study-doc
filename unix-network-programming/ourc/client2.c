#include "common.h"

#define SERV_ADDR "127.0.0.1"
#define SERV_PORT 3000
#define BUF_LEN 1024

#define MAX(a, b) (a)>(b)?(a):(b)

ssize_t writen(int fd, const void *vptr, size_t n)
{
    size_t nleft;
    ssize_t nwriten;
    const char *ptr;

    ptr = vptr;
    nleft = n;
    while(nleft>0) {
        if((nwriten = write(fd, ptr, nleft)) <= 0) {
            if(nwriten < 0 && errno == EINTR) {
                nwriten = 0;        /*interrupt by signal*/
            } else {
                return -1;
            }
        }

        nleft -= nwriten;
        ptr += nwriten;
    }
    return n;
}

void str_cli(FILE *fp, int sockfd)
{
    int maxfdp;
    fd_set rset;
    char sendbuf[BUF_LEN] = {0};
    char recvbuf[BUF_LEN] = {0};

    FD_ZERO(&rset);
    while(1) {
        FD_SET(fileno(fp), &rset);
        FD_SET(sockfd, &rset);
        maxfdp = MAX(fileno(fp), sockfd) + 1;
        select(maxfdp, &rset, NULL, NULL, NULL);

        if(FD_ISSET(sockfd, &rset)) {
            if(read(sockfd, recvbuf, BUF_LEN) == 0) {    //if the length of the data in kernal buffer > 1024 ?

                printf("EOF\n");
                exit(0);
            } else {
                fputs(recvbuf, stdout);
            }
        }
        if(FD_ISSET(fileno(fp), &rset)) {
            if(fgets(sendbuf, BUF_LEN, fp) == NULL) {
                return;
            }
            writen(sockfd, sendbuf, strlen(sendbuf));
        }
        bzero(sendbuf, BUF_LEN);
        bzero(recvbuf, BUF_LEN);
    }
    return;
}

int main(int argc, char **argv)
{
    int fd;
    struct sockaddr_in servaddr;

    fd = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(SERV_ADDR);
    servaddr.sin_port = htons(SERV_PORT);

    if (connect(fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
        printf("connect error: %s\n", strerror(errno));
        return 0;
    }

    str_cli(stdin, fd);
    return 0;
}
