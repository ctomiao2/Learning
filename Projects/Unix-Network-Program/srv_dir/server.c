#include <sys/socket.h>
#include <netinet/in.h>	/* sockaddr_in{} and other Internet defns */
#include <arpa/inet.h>	/* inet(3) functions */
#include <errno.h>
#include <netinet/tcp.h>

#ifdef	HAVE_SYS_SELECT_H
# include	<sys/select.h>	/* for convenience */
#endif

#include <pthread.h>
#include "../lib/ikcp.h"
#include "../lib/utils.h"
#include "../lib/cpp_funcs.h"

// <user_conv, kcp pointer>
static void *kcp_map = NULL;

pthread_mutex_t lock;

long long int bundle_size = 0;

void* tick_kcp_update(void* arg)
{
    while (1) {
        usleep(1000);

        pthread_mutex_lock(&lock);

        int **key_list = (int**) malloc(sizeof(int*));
        void ***val_list = (void***) malloc(sizeof(void**));
        *key_list = NULL;
        *val_list = NULL;
        int n;
        n = unordered_map_items(kcp_map, key_list, val_list);
        int i;
        for (i = 0; i < n; ++i){
            ikcpcb* kcp = (ikcpcb*) ((*val_list)[i]);
            ikcp_update(kcp, iclock());
        }
        
        pthread_mutex_unlock(&lock);

        if (*key_list != NULL)
          free(*key_list);
        if (*val_list != NULL)
            free(*val_list);
        free(key_list);
        free(val_list);
    }

    return NULL;
}

struct KcpUserData {
    int listenfd;
    struct sockaddr_in cliaddr;
    socklen_t cliaddr_len;
};


int udp_output(const char *buf, int len, ikcpcb *kcp, void *user)
{
    struct KcpUserData* user_data = (struct KcpUserData*) user;
    //printf("udp_output: connfd=%d, size=%d,", user_data->listenfd, len);
    print_decode_kcp_str(buf, len);
    // send to client
    sendto(user_data->listenfd, buf, len, 0, (struct sockaddr*) &user_data->cliaddr, user_data->cliaddr_len);
}

void tcp_srv()
{
    int listenfd, connfd, n;
    socklen_t len;
    struct sockaddr_in servaddr, cliaddr;
    listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(7000);
    bind(listenfd, (const struct servaddr*) &servaddr, sizeof(servaddr));
    listen(listenfd, LISTENQ);
    char buff[MAXLINE];
    //const char *sign_str = " ==> reply from server\n";
    
    while(1) {
        len = sizeof(cliaddr);
        connfd = accept(listenfd, (struct servaddr*) &cliaddr, &len);
        int enable = 1;
        setsockopt(connfd, IPPROTO_TCP, TCP_NODELAY, (void*)&enable, sizeof(enable)); 
        
        printf("tcp connection from %s, port %d\n",
            inet_ntop(AF_INET, &cliaddr.sin_addr, buff, sizeof(buff)),
            ntohs(cliaddr.sin_port));
    
        while ((n=read(connfd, buff, MAXLINE)) > 0) {
            bundle_size += n;
            printf("recv data: len=%d, bundle_size=%lld, %s\n", n, bundle_size, buff);
            //memcpy(buff + n - 1, sign_str, strlen(sign_str) + 1);
            write(connfd, buff, n);
            memset(buff, 0, MAXLINE);
        }
    }
}


void kcp_srv(int max_redundant)
{
    printf("kcp_srv, max_redundant=%d\n", max_redundant);
    int listenfd, n;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len = sizeof(cliaddr);
    if ((listenfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        err_quit("socket error");
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(7000);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    bind(listenfd, (const struct servaddr*) &servaddr, sizeof(servaddr));
    

    char buff[MAXLINE];
    const char *sign_str = " ==> reply from server\n";
    
    pthread_mutex_init(&lock, NULL);

    pthread_t tid;
    pthread_create(&tid, NULL, tick_kcp_update, NULL);

again:
    while ((n=recvfrom(listenfd, buff, MAXLINE, 0, (struct sockaddr*) &cliaddr, &len)) > 0) {
        //printf("client send, size: %d, ", n);
        //printf("kcp connection from %s, port %d\n",
        //    inet_ntop(AF_INET, &cliaddr.sin_addr, buff, sizeof(buff)),
        //    ntohs(cliaddr.sin_port));
        
        bundle_size += n;

        IUINT32 user_conv = get_user_conv_from_kcp_str(buff, n);
        
        struct KcpUserData* user = (struct KcpUserData*) malloc(sizeof(struct KcpUserData));
        user->listenfd = listenfd;
        user->cliaddr = cliaddr;
        user->cliaddr_len = len;
        
        if (kcp_map == NULL)
            kcp_map = unordered_map_create();
        
        ikcpcb *kcp = (ikcpcb*) unordered_map_find(kcp_map, (int)user_conv);
        
        // 上锁
        pthread_mutex_lock(&lock);

        if (kcp == NULL) {
            printf("create new kcp...\n");
            kcp = ikcp_create(user_conv, (void*) user);
            kcp->output = udp_output;
            ikcp_wndsize(kcp, 128, 128);
            ikcp_nodelay(kcp, 1, 10, 2, 1);
            ikcp_max_redundant(kcp, max_redundant);
            printf("kcp max_redundant: %d\n", kcp->max_redu);
            //kcp->rx_minrto = 10;
            //kcp->fastresend = 1;
            ikcp_input(kcp, buff, n);
            unordered_map_insert(kcp_map, (int) user_conv, (void*) kcp);
            ikcp_update(kcp, iclock());
        } else { 
            ikcp_input(kcp, buff, n);
            ikcp_update(kcp, iclock());
        }
        
        printf("client send: size=%d, bundle_size=%lld, ", n, bundle_size);
        print_decode_kcp_str(buff, n);

        // response client
        while (1) {
            int hr = ikcp_recv(kcp, buff, MAXLINE);
            if (hr < 0) break;
            printf("recv data: len=%d, ", hr);
            int i;
            for (i = 0; i < hr; ++i) printf("%c", buff[i]);
            printf("\n");
            //memcpy(buff + hr, sign_str, strlen(sign_str) + 1); 
            ikcp_send(kcp, buff, hr);
            ikcp_update(kcp, iclock());
        }

        //解锁
        pthread_mutex_unlock(&lock);

        memset(buff, 0, MAXLINE);
    }

    if (n < 0 && errno == EINTR) {
        printf("error...\n");
        goto again;
    }
    else {
        printf("interrupt: n=%d\n", n);
        int **key_list = (int**) malloc(sizeof(int*));
        void ***val_list = (void***) malloc(sizeof(void**));
        *key_list = NULL;
        *val_list = NULL;
        int n;
        n = unordered_map_items(kcp_map, key_list, val_list);
        int i;
        for (i = 0; i < n; ++i){
            ikcpcb* kcp = (ikcpcb*) ((*val_list)[i]);
            ikcp_release(kcp);
        }
        
        if (*key_list != NULL)
            free(*key_list);
        if (*val_list != NULL)
            free(*val_list);
        free(key_list);
        free(val_list);
    
        if (n < 0)
            err_quit("socket read error");

        close(listenfd);
    }
}


int main(int argc, char **argv)
{
    char *mode = "0";
    if (argc > 1)
        mode = argv[1];
    
    while(1) {
        if (*mode == '0')
            tcp_srv();
        else if (*mode == '1')
            kcp_srv(0);
        else if (*mode > '1' && *mode <= '9')
            kcp_srv(*mode - '1');
        else
            err_quit("invalid mode\n");

    }
    
    pthread_mutex_destroy(&lock);

    exit(0);
}
