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
	
	

