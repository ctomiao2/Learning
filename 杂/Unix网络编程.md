**几个套接字基础接口：**

	struct sockaddr_in serv;
	// 从进程到内核的套接字函数: bind、connect、sendto
	connect(sockfd, (sockaddr *) &serv, sizeof(serv))
	// 从内核到进程的套接字函数: accept、recvfrom、getscoketname、getpeername

**主机字节序和网络字节序转换函数：**

小端存储: 低字节在低地址，高字节在高地址, eg: 

	union {
		short s;
		char c[sizeof(short)];
	} un;
	un.s = 0x0102;
若为小端则: un.c[0] == 2 && un.c[1] == 1，若为大端则: un.c[0] == 1 && un.c[1] == 2；网络字节序为大端存储, 主机可能是大端也可能是小端存储。

	#include <netinet/in.h>
	
	uint16_t htons(uint16_t host16bitvalue);  // h表示host, n表示network, s表示short, 16位的端口号用此表示
	uint32_t htonl(uint32_t host32bitvalue); // 同上, l表示long, 32位的ip地址用此表示
	uint16_t ntohs(uint16_t net16bitvalue);
	uint32_t ntohl(uint32_t net32bitvalue);

**地址转换函数：**

inet\_aton、inet\_addr、inet\_ntoa在点分十进制数串(如 "10.254.48.216")与它长度为32的网络字节序二进制值之间转换IPv4地址, 其中a表示ascii，n表示numeric。

inet\_pton和inet\_ntop对于IPv4和IPv6地址都适用，其中p表示presentation, n表示numeric。
	
旧的接口：
	#include <arpa/inet.h>
	
	int inet_aton(const char *strptr, struct in_addr *addrptr); // 字符串有效则为1, 否则为0
	
	in_addr_t inet_addr(const char *strptr); //若字符串有效则为32位二进制网络字节序的IPv4地址, 否则为INADDR_NONE, **已废弃**
	
	char *inet_ntoa(struct in_addr inaddr); // 返回指向一个点分十进制数串的指针
	
最新接口：

	int inet_pton(int family, const char *strptr, void *addrptr); //若成功则返回1, 非有效表达式为0, 出错为-1
	
	const char *inet_ntop(int family, const void *addrptr, char *strptr, size_t len); //若成功则为指向结果的指针, 出错则为NULL

其中family取AF_INET或AF_INET6。

**socket函数：**

	#inlcude <sys/socket.h>
	int socket(int family, int type, int protocol);
	// family: AF_INET、AF_INET6...
	// type: SOCK_STREAM字节流套接字、SOCK_DGRAM数据报套接字、SOCK_SEQPACKET有序分组套接字、SOCK_RAW原始套接字
	// protocol: IPPROTO_TCP、IPPROTO_UDP、IPPROTO_SCTP

	                                                              TCP服务器
																  socket()
																	 ||
																   bind()
																	 ||
																  listen()
                                                                     ||
                                                                   accept()
																	 || 一直阻塞到客户端连接到达
		TCP客户端                                                     ||
		socket()                                                     ||
		   ||                  TCP三次握手                            ||
		connect() ---------------------------------------------------||
           ||                  数据请求
		write() ---------------------------------------------------->read()
                               数据应答                               || 处理请求
        read() <-----------------------------------------------------
		                       文件结束通知
		close() ----------------------------------------------------> read()
																	  close()
	// 成功返回0, 出错返回-1
	int connect(int sockfd, const struct sockaddr *servaddr, socklen_t addrlen)
	
	// 成功返回0, 出错返回-1, ip和端口都可选，如不指定则由内核选择。
	int bind(int sockfd, const struct sockaddr *myaddr, socklen_t addrlen) 
	
	// listen函数仅由TCP服务器调用, 当socket函数创建一个套接字时它被假设为一个主动套接字即
	// 它是一个将调用connect发起连接的客户套接字。listen函数将一个未连接的套接字转换成一个被动套接字，
	// 指示内核应接受指向该套接字的连接请求，调用listen将导致套接字从CLOSED状态转换到LISTEN状态。
	// 内核为每一个给定的监听套接字维护了两个队列：
	// 1)未完成连接队列: 服务器正在等待完成相应的TCP三路握手过程, 这些套接字处于SYN_RCVD状态。
	// 2)已完成连接队列, 已完成TCP三路握手, 这些套接字处于ESTABLISHED状态。
	// backlog指定了两种队列之和的最大数量, 当队列满时, TCP服务器将忽略客户端发来的SYN(如果返回RST将导致客户端终止而不会重试)
	int listen(int sockfd, int backlog)  // 成功返回0, 出错则为-1
	
	// 成功则返回非负描述符, 出错则-1。用于从已完成连接队列队头返回下一个已完成连接，如果已完成连接队列为空，
	// 那么进程被投入睡眠（假定套接字为默认的阻塞方式）, 第一个参数sockfd为前面的socket()创建的及bind函数
	// 和listen函数的第一个sockfd参数, 返回已连接队列中的代表与某个客户端连接的描述符。
	int accept(int sockfd, struct sockaddr *cliaddr, socklen_t *addrlen);

**fork:**

	#include <unistd.h>
	
	pid_t fork(void); // 子进程中返回0, 父进程中返回子进程id, 出错则返回-1

父进程中调用fork之前打开的所有描述符在fork返回之后由子进程分享，父进程调用accept之后调用fork, 所接受的已连接套接字随后就在父进程与子进程之间共享。fork的两种典型用法：
	
1. 一个进程创建一个自身的副本, 这样每个副本都可以在另一个副本执行其他任务的同时处理各自的某个操作, 这是网络服务器的典型用法。
	
2. 一个进程想要执行另一个程序，进程调用fork创建一个自身的副本然后子进程调用exec把自身替换成新的程序，这是Shell之类程序的典型用法。存放在硬盘上的可执行程序文件能够被unix执行的唯一方法是由一个现有进程调用exec函数将当前进程映像替换成新的程序文件，该新程序通常从main函数开始执行，进程ID并不改变，称调用exec的进程为调用进程(calling process)，新执行的程序为新程序(new program), 注意是新程序而不是新进程，因为exec并没有创建新的进程。
	

		#include <unistd.h>
	
		int execl(const char *pathname, const char *arg0, .../* (char*) 0 */);
		int execv(const char *pathname, char *const argv[]);
		int execle(const char *pathname, const char *arg0, .../* (char*)0, char *const envp[] */);
		int execve(const char *pathname, char *const argv[], char *const envp[]);
		int execlp(const char *filename, const char *arg0, ... /* (char *) 0 */);
		int execvp(const char *filename, char *const argv[]);

调用关系图：

![](./img/unix-exec.png)

	
	int main(int argc, char **argv)
	{
		int listenfd, connfd;
		socklen_t len;
		struct sockaddr_in servaddr, cliaddr;
		listenfd = socket(AF_INET, SOCK_STREAM, 0);
		bzero(&servaddr, sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		servaddr.sin_port = htons(7000);
		bind(listenfd, (const struct servaddr*) &servaddr, sizeof(servaddr));
		listen(listenfd, LISTENQ);
		while (1){
			connfd = accept(listenfd, (struct servaddr*) &cliaddr, &len);
			// fork前只有父进程引用了listenfd、connfd，计数为1
			if (fork() == 0) {
				// 此时listenfd、connfd引用计数都为2
				close(listenfd); // 子进程中执行完后listenfd引用计数为1
				do_something();
				close(connfd);  // 子进程中执行完后connfd引用计数为0即被销毁
				exit(0)
			}
			close(connfd); // 执行完后引用计数为1, 注意这是在父进程中执行
		}
	}

