#include "unp.h"

void str_cli(FILE *fp, int sockfd)
{
    char recvline[MAXLINE], sendline[MAXLINE];
    while (fgets(sendline, MAXLINE, fp) > 0){
        write(sockfd, sendline, strlen(sendline));
        size_t n;
        if ((n=read(sockfd, recvline, sizeof(recvline)) < 0))
            err_sys("read socker error\n");
        printf("%s", recvline);
        //fputs(recvline, stdout);
    }
}

int main(int argc, char **argv)
{
    int sockfd, n;
    char recvline[MAXLINE+1];
    struct sockaddr_in servaddr;
    if (argc != 2) err_quit("usage: client <ip>");
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        err_sys("socket error");
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(7000);
    
   
    
    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0)
        err_quit("inet_pton error for %s", argv[1]);

    int connfd;
    if (connect(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) < 0)
        err_sys("connect error");
    
    str_cli(stdin, sockfd);

    exit(0);
}
