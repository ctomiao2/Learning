#include <sys/socket.h>
#include <netinet/in.h>	/* sockaddr_in{} and other Internet defns */
#include <arpa/inet.h>	/* inet(3) functions */
#include <errno.h>
#include <stdio.h>
#include <pthread.h>
#include "../lib/ikcp.h"
#include "../lib/utils.h"
#include "../lib/cpp_funcs.h"

static int MAX_LOOP = 200;

// 记录序号发送的时间
//static long long int snd_stats[4096];
// 记录序号收到的时间
//static long long int recv_stats[4096];

void fill_recv_stats(const char *recvline, int n, const char* output_filename)
{
    int num = 0;
    long long int t = iclock();
    FILE *fp = fopen(output_filename, "a+");
    int i = 0;
    for (i = 0; i < n; ++i) {
        if (*(recvline+i) == '\n') {
            if (num > 0){ 
                //recv_stats[num] = t;
                fprintf(fp, "%d:%ld\n", num, t);
            }
            num = 0;
        } else {
            num = 10*num + (int)(*(recvline+i)) - (int)'0';
        }
    }
    if (num > 0) {
        //recv_stats[num] = t;
        fprintf(fp, "%d:%lld\n", num, t);
    }
    fclose(fp);
}



/***************************************** tcp funcs ********************************************/
// 开个线程模拟间歇发送数据, 每30ms发送一次
void* tcp_simulate_send_message(void *arg) {
    int sockfd = (int)arg;
    char msg[10];
    //memset(recv_stats, 0, 4096);
    printf("\ntcp start send message, MAX_LOOP=%d...\n", MAX_LOOP);
    
    FILE *fp = fopen("tcp_snd_stats.dat", "a+");
    int i = 0;
    for (i = 0; i < MAX_LOOP; ++i) {
        sprintf(msg, "%d\n", i + 1);
        //for (int j=0; j < 10; ++j) printf("%d", msg[i]);
        //printf("\n");
        fprintf(fp, "%d:%lld\n", i+1, (long long int)iclock());
        write(sockfd, msg, strlen(msg));
        memset(msg, 0, 10);
        usleep(20000);
    }
    fclose(fp);
    printf("\ntcp finish send message...\n");
}

void tcp_cli(const char* srv_addr)
{
    int sockfd, n;
    char recvline[MAXLINE+1];
    struct sockaddr_in servaddr;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        err_quit("socket error");
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(7000);
    
    //setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (int[]){1}, sizeof(int));
    //setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (int[]){1}, sizeof(int));

    if (inet_pton(AF_INET, srv_addr, &servaddr.sin_addr) <= 0)
        err_quit("inet_pton error");

    if (connect(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) < 0)
        err_quit("connect error");
    
    remove("tcp_snd_stats.dat");
    remove("tcp_rcv_stats.dat");

    pthread_t send_msg_tid;
    pthread_create(&send_msg_tid, NULL, tcp_simulate_send_message, sockfd);

    while (1) {
        if ((n=read(sockfd, recvline, MAXLINE)) < 0)
            err_quit("read socket error\n");
        printf("recv data: len=%d, %s", n, recvline);
        fill_recv_stats(recvline, n, "tcp_rcv_stats.dat");
        memset(recvline, 0, MAXLINE);
    }
}

/******************************************* kcp funcs*****************************************/

struct KcpUserData {
    int sockfd;
    struct sockaddr_in servaddr;
};

int udp_output(const char *buf, int len, ikcpcb *kcp, void *user)
{
    struct KcpUserData* user_data = (struct KcpUserData*) user;
    //printf("udp_output: size=%d, ", len);
    //print_decode_kcp_str(buf, len);
    // send msg to server
    sendto(user_data->sockfd, buf, len, 0, (struct sockaddr*) &user_data->servaddr, sizeof(user_data->servaddr));
}

void* tick_kcp_update(void *arg)
{
    ikcpcb *kcp = (ikcpcb*) arg;
    while (1) {
        usleep(20000);
        ikcp_update(kcp, iclock());
    }

    return NULL;
}

void *kcp_simulate_send_message(void *arg)
{
    ikcpcb *kcp = (ikcpcb*) arg;
    char msg[10];
    //memset(recv_stats, 0, 4096);
    printf("\nkcp start send message...\n");
    FILE *fp = fopen("kcp_snd_stats.dat", "a+");
    int i;
    for (i = 0; i < MAX_LOOP; ++i){
        sprintf(msg, "%d", i+1);
        long long int t = iclock();
        fprintf(fp, "%d:%lld\n", i+1, t);
        ikcp_send(kcp, msg, strlen(msg));
        ikcp_update(kcp, t);
        memset(msg, 0, 10);
        usleep(20000);
    }
    fclose(fp);
    printf("\nkcp finish send message...\n");
    while (1) usleep(100000);
}

void kcp_cli(const char* srv_addr)
{
    int sockfd, n;
    char recvline[MAXLINE+1];
    struct sockaddr_in servaddr;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        err_quit("socket error");
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(7000);
    
    if (inet_pton(AF_INET, srv_addr, &servaddr.sin_addr) <= 0)
        err_quit("inet_pton error");

    struct KcpUserData *user = (struct KcpUserData*) malloc(sizeof(struct KcpUserData));
    user->servaddr = servaddr;
    user->sockfd = sockfd;
    
    IUINT32 uid = (IUINT32) guid();
    ikcpcb *kcp = ikcp_create(uid, (void*)user);
    kcp->output = udp_output;
    ikcp_wndsize(kcp, 128, 128);
    ikcp_nodelay(kcp, 1, 10, 2, 1);
    kcp->rx_minrto = 20;
    kcp->fastresend = 1;
    
    remove("kcp_snd_stats.dat");
    remove("kcp_rcv_stats.dat");

    pthread_t send_msg_tid;
    pthread_create(&send_msg_tid, NULL, kcp_simulate_send_message, kcp);

    pthread_t kcp_update_tid;
    pthread_create(&kcp_update_tid, NULL, tick_kcp_update, kcp);
    
    while (1){

        if ((n=recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL)) < 0){
            ikcp_release(kcp);
            err_quit("read socker error\n");
        }
        
        //printf("server response, size: %d, ", n);
        //print_decode_kcp_str(recvline, n);

        ikcp_input(kcp, recvline, n);
        //ikcp_update(kcp, iclock());

        //char recved = 0;
        // response client
        while (1) {
            int hr = ikcp_recv(kcp, recvline, MAXLINE);
            if (hr < 0) break;
            //recved = 1;
            printf("recv data: ");
            int i;
            for (i = 0; i < hr; ++i) printf("%c", recvline[i]);
            fill_recv_stats(recvline, hr, "kcp_rcv_stats.dat");
            printf("\n");
        }

    }
}

int main(int argc, char **argv)
{
    if (argc < 2) 
        err_quit("usage: client <ip> <mode> (0 tcp; 1 kcp; Default 0) <max_loop>");
    
    char *mode = "0";
    if (argc > 2)
        mode = argv[2];

    if (argc > 3)
        MAX_LOOP = atoi(argv[3]);
    
    printf("argc=%d, MAX_LOOP=%d\n", argc, MAX_LOOP);

    if (*mode == '0')
        tcp_cli(argv[1]);
    else if (*mode == '1')
        kcp_cli(argv[1]);
    else
        err_quit("invalid mode\n");

    exit(0);
}
