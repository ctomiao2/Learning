#include "unp.h"
#include <time.h>

void response(int connfd)
{
    char buff[MAXLINE];
    const char *sign_str = " ==> reply from server\n";
    size_t n;
    
again:
    while ((n=read(connfd, buff, MAXLINE)) > 0) {
        printf("read succ, n: %d, %s", n, buff);
        memcpy(buff+n-1, sign_str, strlen(sign_str)+1); 
        writen(connfd, buff, n+strlen(sign_str));
    }
    
    printf("read socket: %d\n", n);

    if (n < 0 && errno == EINTR)
        goto again;
    else if (n < 0)
        err_sys("socket read error");
}

int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t len;
    struct sockaddr_in servaddr, cliaddr;
    //char buff[MAXLINE];
    //time_t ticks;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(7000);
    bind(listenfd, (const struct servaddr*) &servaddr, sizeof(servaddr));
    listen(listenfd, LISTENQ);
    char buff[MAXLINE];
    
    while(1) {
        len = sizeof(cliaddr);
        connfd = accept(listenfd, (struct servaddr*) &cliaddr, &len);
        
        printf("connection from %s, port %d\n",
            inet_ntop(AF_INET, &cliaddr.sin_addr, buff, sizeof(buff)),
            ntohs(cliaddr.sin_port));
        
        if (fork() == 0) {
            close(listenfd);
            response(connfd);
            exit(0);
        }
        close(connfd);
    }
    exit(0);
}
