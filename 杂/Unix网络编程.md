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
	
- **在收到tcp三次握手的第二个分节后，客户端的connect返回，在收到第三个分节后服务端的accept才会返回。**

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


**getsockname、getpeername：**

	#include <sys/socket.h>
	
	int getsockname(int sockfd, struct sockaddr *localaddr, socklen_t *addrlen);
	int getpeername(int sockfd, struct sockaddr *peeraddr, socklen_t *addrlen);

- 在没有调用bind的tcp客户端上, connect返回成功后getsockname用于返回由内核赋予该连接的本地ip地址和本地端口号；
	
		struct sockaddr_in saddr;
		socklen_t slen = sizeof(saddr);
		getsockname(sockfd, (SA*) &saddr, &slen);

- 以端口号0调用bind后, getsockname返回由内核赋予的本地端口号；

- 当一个服务器由调用过accept的某个进程通过调用exec执行程序时，使用getpeername获取客户身份。

tcp客户端example:
 
	// tcp客户端 client.c
	#include "unp.h"
	
	void str_cli(FILE *fp, int sockfd)
	{
	    char recvline[MAXLINE], sendline[MAXLINE];
	    while (fgets(sendline, MAXLINE, fp) > 0){
	        write(sockfd, sendline, strlen(sendline));
	        size_t n;
	        if ((n=read(sockfd, recvline, sizeof(recvline)) <= 0))
	            err_quit("server termiated\n");
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


tcp服务端example:

	// tcp服务端 server.c
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


当客户端中止程序时（即输入CTRL-D），client.c的fgets将返回0, 从而触发main函数里的exit(0)，进程终止要处理的工作是有内核关闭
所有之前打开的描述服，这会导致客户端想服务端发送FIN及服务端返回ACK。此时服务器处于CLOSE\_WAIT状态，客户端处于FIN\_WAIT\_2状态；
服务端阻塞于read调用，在收到客户端发来的FIN时read返回0从而触发子进程的exit(0), 注意此时父进程仍然在主循环不会退出。 子进程的关闭将导致关闭对应的连接描述符，这将导致服务端向客户端发送FIN以及客户端的ACK。此时连接完全终止，客户端进入TIME\_WAIT状态。
**服务器子进程终止时内核将给父进程发送SIGCHLD信号，如不在程序捕获该信号将默认忽略该信号，将导致子进程进入僵死状态。**


**signal：**

- 由一个进程发给另一个进程（或自身）；
- 由内核发给某个进程。

可通过sigaction函数设定一个信号的处理，信号处理函数原型：
	
	void handler(int signo);
	# tips: 关于函数指针
	# 以函数声明void * (*(*func)(int))[10]为例, 从内至外分析
	# (*func)(int)表示func是一个函数指针, 参数是int类型, 那么func代表的函数返回值是什么呢？
	# *(*func)(int)最外层的*表示func代表的函数是返回一个指针类型，那么这个指针是什么类型的指针呢？
	# 最外面的是void * [10]，因此返回一个指向void* [10]的指针
	# 再举个例子：int* (*func2)(int)[10]，不难分析出func2是一个返回值为int* [10]，参数是int类型的函数指针，但是数组int* [10]是不能直接做为返回类型声明的，因此会编译失败。
	# 最后一个例子：int(* (*func3)(int))[10]，func3的参数是int类型，返回值是一个指向int[10]的指针。
	
	# 最后再回到signal的函数声明：
	void(*signal(int, void (*)(int)))(int);
	# 最里面层为：*signal(int, void (*)(int))，表示signal是个函数指针，第一个参数为int，第二个参数是返回类型为
	# void参数类型是int的函数指针，最外层是void (int)表示signal的返回值也是一个void (int)类型的函数指针，因此上
	# 面的定义可等价于：
	typedef void handleFunc(int);
	handleFunc* signal(int, handleFunc *func);

注意：**SIGKILL和SIGSTOP信号不能被捕获**。

	// tcp服务端 server.c
	#include "unp.h"
	#include <time.h>
	
	typedef void sigHandler(int);
	
	// 系统signal包装函数
	sigHandler* signal(int signo, sigHandler *func)
	{
		struct sigaction act, oact;
		act.sa_handler = func;
		// sa_mask为阻塞信号集，表示该信号处理函数func在被调用时, 里面的信号将被阻塞递交
		// 这里设置为空表示不阻塞任何额外信号
		sigemptyset(&act.sa_mask);
		act.sa_flags = 0;
		// 由signo信号中断的系统调用将由内核自动重启, 但是SIGALRM例外
		// 因为SIGALRM信号通常是I/O操作设置超时，这种情况下我们希望被阻塞的系统调用能被信号中断掉
		if (signo == SIGALRM){
		#ifdef SA_INTERRUPT
			act.sa_flags |= SA_INTERRUPT;
		#endif
		} else {
		#ifdef SA_RESTART
			act.sa_flags |= SA_RESTART;
		#endif
		}
		
		if (sigaction(signo, &act, &oact) < 0)
			return SIG_ERR;
		return oact.sa_handler;
	}

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
	
	// 信号处理函数（有问题版本）
	void sig_chld(int signo)
	{
		pid_t pid;
		int stat;
		pid = wait(&stat);
		return;
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
		
		//处理SIGCHLD信号
		signal(SIGCHLD, sig_chld);
		
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


注意：
 
1. 如果一个信号在被阻塞期间产生了一次或多次，那么该信号在解阻塞之后通常只递交一次，即Unix信号默认是不排队的。 可以使用
   sigprocmask函数选择性地阻塞或解阻塞一组信号，比如在一段临界区代码中防止捕获某些信号；

2. 若注释掉 act.sa\_flags |= SA\_RESTART; 则客户端在终止时，服务端子进程会收到FIN导致exit，于是父进程收到SIGCHLD，而此前
   父进程一直阻塞于慢系统调用accept，因为该信号将导致内核中断该accept系统调用(在信号处理函数的return或结尾的地方中断)，返回EINTR，而父进程并未处理这种错误，在某些系统会直接终止，而某些系统则因为accept出错提前返回将不停执行后续的fork逻辑部分。act.sa\_flags |= SA\_RESTART未注释，则被SIGCHLD信号中断的accept系统调用将被内核自动重启，从而保证进程正常运行。但是有
   可能某些系统并不支持重启被信号中断的系统调用，因此可移植地改法是在程序里处理被信号中断的情况：
		
		... // 省略
		while(1) {
		    len = sizeof(cliaddr);
		    if ((connfd = accept(listenfd, (struct servaddr*) &cliaddr, &len)) < 0) {
				if (errno == EINTR)
					continue; // 自动重启
				else
					err_sys("accept error");
			}
		    
		    ... //省略
		}

	对于accept、read、write、select、open之类函数可以这样处理，但connect不能重启，如果connect返回EINTR，我们不能再次调用它，否则将立即返回一个错误，当connect被一个捕获的信号中断而且不自动重启时，我们必须调用select来等待连接完成。


**wait与waitpid：** 都可用来清理已终止子进程

	#include <sys/wait.h>
	// 均返回：若成功则为进程ID，若出错则为0或-1
	// 如果调用wait的进程没有已终止的子进程，不过有一个或多个子进程正在执行，则阻塞到有子进程终止为止
	pid_t wait(int *statloc);

	// pid可指定等待哪个子进程，若为-1则表示等待第一个终止的进程，options允许指定附加选项，常用的是WNOHANG表示在没有已终止的子进程时不要阻塞
	pid_t waitpid(pid_t pid, int *statloc, int options);

当同时有多个客户端连接时，建立一个信号处理函数并在其中调用wait并不足以防止出现僵死进程。因为unix信号一般是不排队的，如果信号处理函数正在执行时依次收到其他子进程的信号，那么当信号处理函数执行完毕后只会再执行一次，从而可能导致产生僵死进程，正确地解决办法是使用waitpid。

	void sig_chld(int signo)
	{
		pid_t pid;
		int stat;
		while ((pid = waitpid(-1, &stat, WNOHANG)) > 0)
			printf("child %d terminated\n", pid);
		return;
	}

使用上述信号处理函数能work的原因在于：使用了循环去逐个wait已终止的子进程，因此所有已终止的子进程都会被处理，wait能否也使用循环呢？答案当然是否定的，因为wait做不到在没有已终止的子进程时不阻塞，也就是说如果对wait使用循环那么当没有已终止的子进程时父进程会被阻塞，这显然是不行的，而waitpid的WNOHANG选项可以保证在没有已终止的子进程时不阻塞。


**结论：**

1. 当fork子进程时必须捕获SIGCHLD信号；
2. 当捕获信号时，必须处理被中断的系统调用；
3. SIGCHLD的信号处理函数必须正确编写，应使用waitpid函数以免留下僵死进程。


**accept返回前连接终止：** tcp三路握手完成后tcp客户端却发送了一个RST(复位)，服务器进程调用accept的时候RST到达，POSIX指出在这种情况返回的errno必须是ECONNABORTED（但不同系统在具体实现的时候不尽相同），服务器进程遇到这种错误时就可以忽略它再次调用accept就行。

**服务器进程终止：** 使用kill命令杀掉tcp服务端子进程，子进程中所有打开的描述符都被关闭，将导致向客户发送一个FIN，客户tcp响应ACK，但是按此前设计的tcp客户端str\_cli函数，这时仍然阻塞于fgets调用等待用户输入，一旦用户输入后tcp客户将把数据发送给服务器，tcp允许这么干是因为tcp客户收到FIN只是表示服务器进程已经关闭的连接的服务器端不会再往客户发送任何数据而已，但FIN的接收并没有告知tcp服务器进程已经终止（本例中确实是终止了），因为tcp客户认为服务端进程仍然可以接收数据。**当tcp服务端接收到来自客户的数据时，因为该客户关联的套接字进程已经终止，于是相应一个RST。但是客户端看不到这个RST**，因为此时客户端正阻塞于read调用，而根据之前接收的FIN，read调用立即返回0(表示EOF)，于是执行err_quit函数终止客户端。这个例子问题在于tcp客户端在应对两个描述符————套接字和用户输入，它不能单纯地阻塞在这两个源中某个特定源的输入上，也就是说即使服务端已经发送FIN给客户端了，但客户端可能正阻塞于用户输入导致不能立马获知已收到FIN。

**SIGPIPE:** 向某个已接收到RST的套接字执行写操作时，内核向该进程发送一个SIGPIPE信号，该信号的默认行为是终止进程，因此进程必须捕获它或者设置忽略该信号以免进程被不情愿地终止。 但无论该进程是捕获了该信号并从其信号处理函数返回还是简单地忽略该信号，写操作都将返回EPIPE错误。 **写一个已接收了FIN的套接字将引发服务端发送RST, 写一个已接收了RST的套接字将引发SIGPIPE信号**。

	#include "unp.h"
	
	void str_cli(FILE *fp, int sockfd)
	{
	    char recvline[MAXLINE], sendline[MAXLINE];
	    while (fgets(sendline, MAXLINE, fp) > 0){
			write(sockfd, sendline, 1);
			sleep(1);
	        write(sockfd, sendline+1, strlen(sendline)-1);
	        size_t n;
	        if ((n=read(sockfd, recvline, sizeof(recvline)) <= 0))
	            err_quit("server termiated\n");
	        printf("%s", recvline);
	        //fputs(recvline, stdout);
	    }
	}

客户端正阻塞于fgets，杀掉服务端子进程，然后客户端键入一行文本，第一次write会引发RST, 第二次写会引发SIGPIPE信号，由于进程并未捕获该信号，因此进程将被终止。可行的做法是直接将SIGPIPE设置SIG_IGN，然后在write后处理返回的EPIPE错误。

**服务器主机崩溃：** tcp客户端会在write后阻塞于read调用，但由于服务端不可达，因此客户端会一直重传数据直到超时，然后readline调用返回错误ETIMEOUT，如果某个中间路由器判定服务器主机已不可达，则响应一个ICMP消息，客户端返回错误EHOSTUNREACH或ENETUNREACH。如果想要不主动向服务端发送数据就能检测出服务器主机崩溃，那么需要使用SO_KEEPALIVE套接字选项。

**服务器主机崩溃后重启：** 服务器主机重启后，它的tcp丢失了崩溃前的所有连接信息，因此服务器tcp对于收到来自客户的数据分节响应一个RST。tcp客户端收到该RST时，read调用返回ECONNRESET错误。

**服务器主机关机：** unix系统关机时，init进程通常会先给所有进程发送SIGTERM信号（可被捕获），等待一段固定时间（通常5到20秒之间），然后给所有仍在运行的进程发送SIGKILL信号（不可被捕获）并终止。如果我们忽略SIGTERM信号，那么服务器将由SIGKILL信号终止，而SIGTERM的默认处置也是终止进程，即我们必须捕获SIGTERM才不至于被意外终止。


## I/O复用： ##

定义：内核在进程指定的一个或多个描述符就绪时通知给进程的能力即为I/O复用。

几种I/O模型：

- 阻塞式I/O，之前的所有例子都是这种类型；
- 非阻塞式I/O，也称轮询，较少使用；
- I/O复用，阻塞于select调用，但与普通阻塞式I/O区别是select可以监听多个描述符而不是一个；
- 多线程阻塞式I/O，对每个套接字开个线程阻塞调用
- 信号驱动式I/O，内核在描述符就绪时发送SIGIO信号通知进程（该阶段非阻塞），进程可以在信号处理函数中调用recvfrom读取数据报。
- 异步I/O模型，告知内核启动某个操作，并让内核在整个操作（包括数据从内核复制到进程缓冲区）完成后通知进程。与信号驱动I/O模型区别是：信号驱动I/O由内核通知我们何时可以启动一个I/O操作，而异步I/O模型则是由内核通知我们I/O操作何时完成。

上述几种模型除了异步I/O模型外，其余几种模型都是在第一阶段即描述符就绪阶段有区别，而读取数据报阶段则相同，它们都是阻塞式I/O模型，因为在recvfrom阶段是阻塞的。

![](./img/unix-io.png)

**select：** 该函数允许进程指示内核等待多个事件中的任何一个发生，并只在有一个或多个事件发生时或者超时后才唤醒进程。

	#include <sys/select.h>
	#include <sys/time.h>
	
	int select(int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset, const struct timeval *timeout);
	
	struct timeval {
		long tv_sec;  // 秒
		long tv_usec; // 微秒
	}
timeout参数告知内核等待所指定的描述符就绪的最大时长。 

- timeout为NULL表示永远等待下去，直到有一个描述符准备好I/O；
- 等待固定时间，在有一个描述符就绪时返回，但是不超过timeout指定的秒数与微秒数；
- 根本不等待，检查描述符后立即返回，这种方式为轮询(polling)，将timeout的秒与微秒都设为0。

readset、writeset、exceptset指定内核测试读、写、异常的描述符，若对其中某个条件不感兴趣可以设置为空指针。

- void FD_ZERO(fd\_set *fdset);   // 清空fdset
- void FD_SET(fd, fd\_set *fdset);  // 打开fdset的第fd个bit的开关
- void FD_CLR(int fd, fd\_set *fdset);  //关闭fdset的第fd个bit的开关
- int FD_ISSET(int fd, fd\_set *fdset); // fdset的第fd个开关是否被打开

maxfdp1指定待测试的描述符个数，其值为待测试的最大描述符加1（maxfd plus 1)，描述符0, 1, 2, ... maxfdp1-1都会被测试。

**描述符就绪条件**：

1) 满足下列任何一个条件时，一个套接字准备好读：

- 该套接字接收缓冲区的数据字节数大于等于套接字接收缓冲区低水位标记大小，可以设置SO_RCVLOWAT套接字选项，对tcp和udp默认为1。对这样的套接字执行读操作不阻塞并将返回一个大于0的值。
- 该连接的读半部关闭（即收到了FIN的tcp连接），对这样的套接字执行读操作不阻塞并且返回0（即EOF）。
- 该套接字是一个监听套接字且已完成的连接数不为0。
- 该套接字有一个错误待处理，对这样的套接字执行读操作不阻塞并返回-1，同时把errno设置成确切的错误条件。

2）满足下列任何一个条件时，一个套接字准备好写：

- 该套接字发送缓冲区中的可用空间字节数大于等于套接字发送缓冲区低水位标记大小，并且该套接字已连接或者该套接字不需要连接（udp套接字），可以使用SO_SNDLOWAT套接字选项来设置该套接字的低水位标记，对tcp和udp而言，其默认值为2048。
- 该连接的写半部关闭，对这样的套接字写将产生SIGPIPE信号。
- 使用非阻塞式connect的套接字已建立连接或者connect已经以失败告终。
- 其上有一个套接字错误待处理，对这样的套接字写将不阻塞且返回-1，同时errno被设置成确切的错误条件。

3）如果一个套接字存在带外数据或者仍然处于带外标记，那么有异常条件待处理。

**注意：** **当select函数返回时将修改由指针readset、writeset、exceptset所指向的描述符集，描述符集内任何与未就绪描述符对应的位都被清为0，因此每次重新调用select函数时都得再次把所有描述符集内关心的为重置为1**。


**改进版的str_cli：**

	#include "unp.h"
	
	void str_cli(FILE *fp, int sockfd)
	{
		int madfdp1;
		fd_set rset;
	    char recvline[MAXLINE], sendline[MAXLINE];
	    FD_ZERO(&rset);
		while (true){
			FD_SET(fileno(fp), &rset);
			FD_SET(sockfd, &rset);
			maxfdp1 = max(fileno(fp), sockfd) + 1;
			select(maxfdp1, &rset, NULL, NULL, NULL);
			
			if (FD_ISSET(sockfd, &rset)) { /* socket is readable */
				if (readline(sockfd, recvline, MAXLINE) == 0)
					err_quit("str_cli: server terminated prematurely");
				fputs(recvline, stdout);
			}
			
			if (FD_ISSET(fileno(fp), &rset)) { /* input is readable */
				if (fgets(sendline, MAXLINE, fp) == NULL)
					return;
				writen(sockfd, sendline, strlen(sendline));
			}
	    }
	}

改版本str_cli仍然有问题，输入端终止后会立即回到main函数执行exit终止进程，但这时可能仍然有请求正在途中。

**shutdown：**

	#include <sys/socket.h>
	
	int shutdown(int sockfd, int howto);

howto:

- SHUT_RD：关闭套接字读，丢掉套接字接收缓冲区中的数据，对一个tcp套接字而言，对端发来的所有数据都被确认然后丢弃。
- SHUT_WR：关闭套接字写，套接字发送缓冲区的数据会被立即发送，后跟tcp的正常连接终止序列，无视tcp套接字的引用计数。
- SHUT_RDWR：读写都关闭，等价于关闭两次SHUT_RD、SHUT_WR

**select服务端程序：**

	#include "unp.h"
	
	int main(int argc, char **argv)
	{
		int listenfd, connfd, i, maxi, maxfd;
		int nready, client[FD_SETSIZE];
		fd_set rset, allset;
		struct sockaddr_in servaddr, cliaddr;
		listenfd = socket(AF_INET, SOCK_STREAM, 0);
		bzero(&servaddr, sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		servaddr.sin_port = htons(7000);
		bind(listenfd, (const struct servaddr*)&serveraddr, sizeof(servaddr));
		listen(listenfd, LISTENQ);
		char buff[MAXLINE];
		maxfd = listenfd;
		maxi = -1;
		for (i = 0; i < FD_SETSIZE; i++)
			client[i] = -1;
		FD_ZERO(&allset);
		FD_SET(listenfd, &allset);
		while (1) {
			rset = allset;
			nready = select(maxfd+1, &rset, NULL, NULL, NULL);
			if (FD_ISSET(listenfd, &rset)) {
				size_t clilen = sizeof(cliaddr);
				connfd = accept(listenfd, (SA*)&cliaddr, &clilen);
				for (i=0; i < FD_SETSIZE; i++) {
					if (client[i] < 0) {
						client[i] = connfd;
						break;
					}
				}
				if (i == FD_SETSIZE)
					err_quit("too many clients");
				FD_SET(connfd, &allset);
				if (connfd > maxfd) maxfd = connfd;
				if (i > maxi) maxi = i;
				if (--nready <= 0) continue;
			}
			for (i=0; i <= maxi; ++i) {
				int sockfd, n;
				if ((sockfd = client[i]) < 0) continue;
				if (FD_ISSET(sockfd, &rset)) {
					if ((n = readline(sockfd, buff, MAXLINE) == 0) {
						close(sockfd);
						FD_CLR(sockfd, &allset);
						client[i] = -1;
					} else {
						writen(sockfd, buff, n);
					}
					if (--nready <= 0)
						break;
				}
			}
		}
	}

该服务端程序仍然不能避免拒绝服务攻击，客户端可以恶意发送一个字节的数据（不是换行符），那么服务端将调用readline读入这个单字节数据，由于readline需要读到换行符或EOF才返回，因此readline一直阻塞，由于服务端使用的是单进程单线程模型，因此所有其他客户端服务都将不可用。解决办法包括：（1）使用非阻塞式I/O；（2）让每个客户由单独的控制线程提供服务（如创建一个子进程或一个线程来服务每个客户）；（3）对I/O操作设置一个超时。 当然，若将readline换成read也不会存在拒绝服务攻击的问题了。


**pselect：**

	#include <sys/select.h>
	#include <signal.h>
	#include <time.h>
	
	int pselect(int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset, const struct timespec *timeout, const.sigset_t *sigmask);
	
**sigprocmask：**

	int sigprocmask(int how, const sigset_t *set, sigset_t *oldset);

sigprocmask用于设置和获取信号掩码（signal mask， 被当前进程屏蔽的信号集）。 若oldset不是NULL，则旧的signal mask被保存到oldset，若set为NULL则signal mask不会有任何改变（即how被忽略），但当前的signal mask仍然被保存到oldset（若oldset不为NULL）。 how的选项如下：

- SIG_BLOCK： 当前signal mask和参数set里的信号都被屏蔽，即增量设置；
- SIG_UNBLOCK：解除屏蔽参数set里的信号，允许set里的信号是非屏蔽的；
- SIG_SETMASK：将signal mask设置成参数set，即全量设置。

	
若sigmask为NULL，则pselect与select等价，否则pselect等价于如下**原子**执行如下代码段：

	sigset_t oldmask, sigmask;
	sigprocmask(SIG_SETMASK, &sigmask, &oldmask);
	select(...);
	sigprocmask(SIG_SETMASK, &oldmask, NULL);
即pselect时会将当前的signal mask替换成参数sigmask，调用完毕后再还原。


考虑这样一个程序段：
	
	static volatile int intr_flag = 0;
	
	void handle_intr() {
		.....  // do something
		intr_flag = 0;
	}
	
	void sig_handler() {
		intr_flag = 1;
	}

	sa.sa_handler = sig_handler;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGINT, &sa, NULL);

	sigset_t new, old, zero;
	sigemptyset(&zero);
	sigemptyset(&new);
	sigaddset(&new, SIGINT);
	sigprocmask(SIG_BLOCK, &new, old); //block SIGINT
	/*********** NOTE-1 ***********/
	if (intr_flag)
		handle_intr(); // handle the signal
	if ((nready = pselect(maxfdp1, readset, writeset, exceptset, timeout, &zero)) < 0) {
		if (errno == EINTR) {
			if (intr_flag)
				handle_intr();
		}
	}

sigprocmask函数会屏蔽掉SIGINT信号，pselect的sigmask是空，即不屏蔽任何信号，因此执行到pselect时SIGINT信号又再次被解除屏蔽。 这段代码的意思是直到pselect调用前SIGINT一直被屏蔽，有几种情况：若在sigprocmask前收到SIGINT信号则会在NOTE-1后面两行将执行handle\_intr；若是在sigprocmask之后pselect之前收到SIGINT信号，则将被忽略；若在pselect调用后收到SIGINT则sig\_handler将被执行，且pselect被打断返回EINTR错误从而执行后面的handle_intr。

**poll**：
	
	#include <poll.h>
	// 返回： 若有就绪描述符则为其数目，若超时则为0，若出错则为-1
	int poll(struct pollfd *fdarray, unsigned long nfds, int timeout);
	
	struct pollfd {
		int fd;  /* descriptor to check */
		short events; /* events of interest on fd */
		short revents;  /* events that occurred on fd */
	}

要测试的条件由events成员指定，函数在相应的revents成员中返回该描述符的状态。events和revents常值:

- POLLIN：普通或优先级带数据可读；
- POLLRDNORM：普通数据可读；
- POLLRDBAND：优先级带数据可读；
- POLLPRI：高优先级数据可读；
- POLLOUT：普通数据可写；
- POLLWRNORM：普通数据可写；
- POLLWRBAND：优先级带数据可写；
- POLLERR：发生错误；
- POLLHUP：发生挂起；
- POLLNVAL：描述符不是一个打开的文件

其中后三个不可作为events值。 

- 所有正规tcp数据与udp数据都被认为是普通数据；
- tcp带外数据被认为是优先级带数据；
- tcp的读半部关闭时（即收到了对端的FIN），被认为是普通数据，随后的读操作将返回0；
- tcp连接存在错误既可被认为是普通数据也可认为是发生错误POLLERR，在随后的读操作将返回-1，并把errno设置成合适的值；
- 在监听套接字上有新的连接可用既可被认为是普通数据也可认为是优先级数据，大多数实现视为普通数据；
- 非阻塞式套接字connect完成被认为是可写。

timeout: 0立即返回不阻塞；>0等待指定毫秒数；INFTIM永远等待。

当发生错误时poll立即返回-1，若定时器到时之前没有任何描述符就绪则返回0，否则返回已就绪的描述符数量，即revents中非0的数量。若对某个pollfd不感兴趣则可将该pollfd的fd设为-1。 使用poll的tcp服务端程序：

	int main(int argc, char **argv)
	{
	    int i, maxi, listenfd, connfd, sockfd;
	    int nready, n;
	    char buf[MAXLINE];
	    socklen_t clilen;
	    const int OPEN_MAX = 1024;
	    struct pollfd client[OPEN_MAX];
	    struct sockaddr_in cliaddr, servaddr;
	    
	    listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	    bzero(&servaddr, sizeof(servaddr));
	    servaddr.sin_family = AF_INET;
	    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	    servaddr.sin_port = htons(7000);
	
	    bind(listenfd, (SA*) &servaddr, sizeof(servaddr));
	
	    listen(listenfd, LISTENQ);
	
	    client[0].fd = listenfd;
	    client[0].events = POLLRDNORM;
	    for (i = 1; i < OPEN_MAX; i++)
	        client[i].fd = -1;
	    maxi = 0;
	
	    while (1) {
	        nready = poll(client, maxi + 1, INFTIM);
	        
	        /* check listenfd */
	        if (client[0].revents & POLLRDNORM) {
	            clilen = sizeof(cliaddr);
	            connfd = accept(listenfd, (SA*) &cliaddr, &clilen);
	            for (i=1; i < OPEN_MAX; ++i) {
	                if (client[i].fd < 0) {
	                    client[i].fd = connfd;
	                    client[i].events = POLLRDNORM;
	                    if (i > maxi) maxi = i;
	                    break;
	                }
	            }
	            
	            if (i == OPEN_MAX)
	                err_quit("too many clients");
	
	            if (--nready <= 0) continue;
	    
	        }
	        
	        /* check client connfd */
	        for (i=1; i <= maxi; ++i) {
	            if ((sockfd = client[i].fd) < 0)
	                continue;
	            if (client[i].revents & (POLLRDNORM | POLLERR)) {
	                if ((n = read(sockfd, buf, MAXLINE)) < 0) {
	                    if (errno == ECONNRESET) {
	                        close(sockfd);
	                        client[i].fd = -1;
	                    } else {
	                        err_sys("read error");
	                    }
	                } else if (n == 0) {
	                    close(sockfd);
	                    client[i].fd = -1;
	                } else {
	                    writen(sockfd, buf, n);
	                }
	
	                if (--nready <= 0)
	                    break;
	            }
	
	        }
	    }
	}



**epoll**：
	
	// 创建一个epoll句柄, size表示监听数目, size并不是限制epoll所能监听的描述符最大个数而是对内核分配数据结构的建议值
	int epoll_create(int size)
	
	// epfd即为epoll_create的返回值, fd为需要监听的文件描述符
	// op取值：EPOLL_CTL_ADD, EPOLL_CTL_DEL, EPOLL_CTL_MOD，分别表示添加、删除和修改对fd的监听事件
	// epoll_event告诉内核监听什么事件
	int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)
	
	// 等待epfd上的io事件，最多返回maxevents个事件，events用来保存从内核得到的事件集合，maxevents告知内核这个events有多大，不能大于epoll_create的size，timeout为超时时间（0立即返回，-1永久阻塞），该函数返回需处理的事件数目，如返回0则表示超时。
	int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout)

	struct epoll_event {
		__uint32_t events;  /* epoll events */
		epoll_data_t data;  /* user data variable */
	}
	
	events可取以下几个宏：
	EPOLLIN：普通数据可读，包括对端SOCKET正常关闭；
	EPOLLOUT：普通数据可写；
	EPOLLPRI：优先级带数据可读；
	EPOLLERR：文件描述符发生错误；
	EPOLLHUP：文件描述符被挂断；
	EPOLLET：将EPOLL设为边缘触发（Edge Triggered模式)
	EPOLLONESHOT：只监听一次事件，当监听完这次事件之后，如果还需要继续监听这个socket需再次把它加入到EPOLL队列。

epoll没有描述符限制，使用一个文件描述符管理多个描述符。epoll对文件描述符的操作有两种模式：

- LT模式（level trigger)：当epoll\_wait检测到描述符事件发生并将此事件通知应用程序，应用程序可以不立即处理该事件，下次调用epoll_wait时会再次响应应用程序并通知此事件；该模式为默认模式，同时支持block和no-block socket。

- ET模式（edge trigger)：当epoll\_wait检测到描述符事件发生并将此事件通知应用程序，应用程序必须立即处理该事件。如果不处理，下次调用epoll\_wait时，不会再次响应应用程序并通知此事件。该模式是高速工作方式，只支持no-block socket。在这种模式下，当描述符从未就绪变为就绪时，内核通过epoll告诉你。然后它会假设你知道文件描述符已经就绪，并且不会再为那个文件描述符发送更多的就绪通知，直到你做了某些操作导致那个文件描述符不再为就绪状态了(比如，你在发送，接收或者接收请求，或者发送接收的数据少于一定量时导致了一个EWOULDBLOCK 错误）

使用epoll的tcp服务端程序：


	#include "unp.h"
	#include <sys/epoll.h>
	
	#define EPOLLEVENTS 100
	
	void epoll_ctrl_event(int epollfd, int fd, int state, int op) {
	    struct epoll_event event;
	    event.events = state;
	    event.data.fd = fd;
	    epoll_ctl(epollfd, op, fd, &event);
	}
	
	void add_event(int epollfd, int fd, int state) {
	    epoll_ctrl_event(epollfd, fd, state, EPOLL_CTL_ADD);
	}
	
	void delete_event(int epollfd, int fd, int state) {
	    epoll_ctrl_event(epollfd, fd, state, EPOLL_CTL_DEL);
	}
	
	void modify_event(int epollfd, int fd, int state) {
	    epoll_ctrl_event(epollfd, fd, state, EPOLL_CTL_MOD);
	}
	
	void handle_accept(int epollfd, int listenfd) {
	    printf("handle_accept...\n");
	    int connfd;
	    struct sockaddr_in cliaddr;
	    socklen_t clilen = sizeof(cliaddr);
	    if ((connfd = accept(listenfd, (SA*) &cliaddr, &clilen)) < 0) {
	        err_quit("accept error");
	    } else {
	        add_event(epollfd, connfd, EPOLLIN);
	    }
	}
	
	void do_read(int epollfd, int fd, char *buf) {
	    printf("do_read, %s\n", buf);
	    int nread;
	    nread = read(fd, buf, MAXLINE);
	    if (nread == -1) {
	        close(fd);
	        delete_event(epollfd, fd, EPOLLIN);
	        err_quit("read error");
	    } else if (nread == 0) {
	        close(fd);
	        delete_event(epollfd, fd, EPOLLIN);
	    } else {
	        // 修改描述符对应事件，改读为写
	        modify_event(epollfd, fd, EPOLLOUT);
	    }
	}
	
	void do_write(int epollfd, int fd, char *buf) {
	    printf("do_write, %s\n", buf);
	    int nwrite;
	    nwrite = write(fd, buf, strlen(buf));
	    if (nwrite == -1) {
	        close(fd);
	        delete_event(epollfd, fd, EPOLLOUT);
	        err_quit("write error");
	    } else {
	        // 改写为读
	        modify_event(epollfd, fd, EPOLLIN);
	    }
	}
	
	void handle_events(int epollfd, struct epoll_event *events, int nready, int listenfd, char *buf)
	{
	    for (int i=0; i < nready; ++i) {
	        int fd = events[i].data.fd;
	        if ((fd == listenfd) && (events[i].events & EPOLLIN))
	            handle_accept(epollfd, listenfd);
	        else if (events[i].events & EPOLLIN)
	            do_read(epollfd, fd, buf);
	        else if (events[i].events & EPOLLOUT)
	            do_write(epollfd, fd, buf);
	    }
	}
	
	int main(int argc, char **argv)
	{
	    int listenfd;
	    char buf[MAXLINE];
	    struct epoll_event events[EPOLLEVENTS];
	    struct sockaddr_in servaddr;
	    
	    listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	    bzero(&servaddr, sizeof(servaddr));
	    servaddr.sin_family = AF_INET;
	    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	    servaddr.sin_port = htons(7000);
	
	    bind(listenfd, (SA*) &servaddr, sizeof(servaddr));
	    
	    listen(listenfd, LISTENQ);
	
	    // create epoll fd
	    int epollfd = epoll_create(1000);
	    
	    add_event(epollfd, listenfd, EPOLLIN);
	
	    while (1) {
	        // 返回准备好的描述符
	        printf("begin epoll_wait...\n");
	        int ret = epoll_wait(epollfd, events, EPOLLEVENTS, -1);        
	        printf("ret: %d\n", ret);
	        handle_events(epollfd, events, ret, listenfd, buf);
	    }
	    close(epollfd);
	}


通过比较不难发现：select/poll在有文件描述符就绪时都要遍历一遍所有文件描述符以确定是哪些描述符就绪，当文件描述符较多时比较低效，而epoll则是实现通过epoll_ctl来注册一个文件描述符，一旦某个文件描述符就绪内核就会激活这个文件描述符事件，应用程序就能立马直到是哪些文件描述符已就绪，而无需遍历其他未就绪的文件描述符。

## UDP ##

不需要建立连接，只需调用sendto发送数据报，调用recvfrom接收数据报。

	#include <sys/socket.h>
	
	ssize_t recvfrom(int sockfd, void *buff, size_t nbytes, int flags, 
		struct sockaddr *from, socklen_t *addrlen);
	
	ssize_t sendto(int sockfd, const void *buff, size_t nbytes, int flags,
		const struct sockaddr *to, socklen_t *addrlen);

前三个参数sockfd、buff、nbytes等同于read和write函数的三个参数，后两个参数类似于accept和connect的最后两个参数。若recvfrom的from为NULL，则addrlen也必须为NULL，表示不关心发送者的协议地址。

udp服务端程序：

	#include "unp.h"

	int main(int argc, char **argv)
	{
	    int sockfd;
	    struct sockaddr_in servaddr, cliaddr;
	    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	    bzero(&servaddr, sizeof(servaddr));
	    servaddr.sin_family = AF_INET;
	    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	    servaddr.sin_port = htons(7000);
	    bind(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr));
	
	    char buf[MAXLINE];
	    socklen_t len = sizeof(cliaddr);
	
	    while (1) {
	        int n = recvfrom(sockfd, buf, MAXLINE, 0, (struct sockaddr*) &cliaddr, &len);
	        printf("recv from client: %s", buf);
	        sendto(sockfd, buf, n, 0, (struct sockaddr*) &cliaddr, len);
	    }
	}


udp客户端程序：

	#include "unp.h"

	int main(int argc, char **argv)
	{
	    int sockfd;
	    struct sockaddr_in servaddr;
	
	    if (argc != 2)
	        err_quit("missing <IPaddress>");
	
	    bzero(&servaddr, sizeof(servaddr));
	    servaddr.sin_family = AF_INET;
	    servaddr.sin_port = htons(7000);
	    inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
	
	    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	    socklen_t len;
	    char sendline[MAXLINE], recvline[MAXLINE+1];
	    while (fgets(sendline, MAXLINE, stdin) != NULL) {
	        sendto(sockfd, sendline, strlen(sendline), 0, (struct sockaddr*) &servaddr, sizeof(servaddr));
			struct sockaddr *preply_addr = (struct sockaddr*) malloc(sizeof(servaddr));
	        int n = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL);
			 if (len != sizeof(servaddr) || memcmp((const void*)&servaddr, (const void*)preply_addr, len) != 0) { 
				printf("reply from %s\n", sock_ntop(preply_addr, len));
 				continue;
			}
	        recvline[n] = '\0';
	        fputs(recvline, stdout);
	    }
	
	    exit(0);
	}

使用connect：不使用connect时，sendto在发送数据报前会先连接服务端（但不会有三次握手），发送完数据报后会再断开连接（也不会有三次挥手），下次发送数据报时重复同样的步骤。 不使用显示connect服务端发生错误时客户端不会收到通知，另外使用显示connect可以减少通信时长。

	#include "unp.h"
	
	int main(int argc, char **argv)
	{
	    int sockfd;
	    struct sockaddr_in servaddr;
	
	    if (argc != 2)
	        err_quit("missing <IPaddress>");
	
	    bzero(&servaddr, sizeof(servaddr));
	    servaddr.sin_family = AF_INET;
	    servaddr.sin_port = htons(7000);
	    inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
	
	    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	    
	    char sendline[MAXLINE], recvline[MAXLINE+1];
	    socklen_t len;
	
	    if (connect(sockfd, (const struct sockaddr*) &servaddr, sizeof(servaddr)) < 0)
	        err_quit("connect error");
	
	    while (fgets(sendline, MAXLINE, stdin) != NULL) {
	        write(sockfd, sendline, strlen(sendline));
	        int n = read(sockfd, recvline, MAXLINE);
	        recvline[n] = '\0';
	        fputs(recvline, stdout);
	    }
	
	    exit(0);
	}

udp的connect无需三次握手，只是一个本地操作，只保存对端的ip地址和端口号。因此当传的服务端地址不对时connect并不会像tcp一样立即返回错误，而是在write后才会返回错误。给一个未绑定端口号的udp套接字调用connect后同时也给该套接字指派一个临时端口。（tcp和udp套接字可共用一个端口，两种端口是独立的）

**udp无流量控制：** 让udp客户端使用for循环调用2000次sendto，最终服务端会丢失很多udp包，实测发现只收到400多条，丢失率大约75%，可以用netstat -s -p udp查看udp接收情况。 udp给某个特定套接字排队的UDP数据报数目受限于该套接字接收缓冲区的大小。可以使用SO_RCVBUF套接字选项修改该值：

	int n = 256 * 1024;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &n, sizeof(n))

设置后上段例子中服务端可以接收更多的udp数据报，不过由于套接字缓冲区存在最大限制，仍然不能从根本上解决问题。


**名字与地址转换：**

	#include <netdb.h>
	
	// 只能查ipv4地址
	struct hostent* gethostbyname(const char *hostname);
	// 由一个二进制ip地址找到对应的主机名
	struct hostent* gethostbyaddr(const char *addr, socklen_t len, int family);

	struct hostent {
		char *h_name;  // official name of host
		char **h_aliased;  // alias names pointer
		int h_addrtype;  // host address type: AF_INET
		int h_length;  // length of address: 4
		char **h_addr_list;  // ptr to array of ptrs with ipv4 addrs
	}
	
