#ifndef __RCP_H__
#define __RCP_H__

#include <stddef.h>
#include <stdlib.h>
#include <assert.h>

//=====================================================================
// 32BIT INTEGER DEFINITION 
//=====================================================================
#ifndef __INTEGER_32_BITS__
#define __INTEGER_32_BITS__
#if defined(_WIN64) || defined(WIN64) || defined(__amd64__) || \
	defined(__x86_64) || defined(__x86_64__) || defined(_M_IA64) || \
	defined(_M_AMD64)
typedef unsigned int ISTDUINT32;
typedef int ISTDINT32;
#elif defined(_WIN32) || defined(WIN32) || defined(__i386__) || \
	defined(__i386) || defined(_M_X86)
typedef unsigned long ISTDUINT32;
typedef long ISTDINT32;
#elif defined(__MACOS__)
typedef UInt32 ISTDUINT32;
typedef SInt32 ISTDINT32;
#elif defined(__APPLE__) && defined(__MACH__)
#include <sys/types.h>
typedef u_int32_t ISTDUINT32;
typedef int32_t ISTDINT32;
#elif defined(__BEOS__)
#include <sys/inttypes.h>
typedef u_int32_t ISTDUINT32;
typedef int32_t ISTDINT32;
#elif (defined(_MSC_VER) || defined(__BORLANDC__)) && (!defined(__MSDOS__))
typedef unsigned __int32 ISTDUINT32;
typedef __int32 ISTDINT32;
#elif defined(__GNUC__)
#include <stdint.h>
typedef uint32_t ISTDUINT32;
typedef int32_t ISTDINT32;
#else 
typedef unsigned long ISTDUINT32;
typedef long ISTDINT32;
#endif
#endif

//=====================================================================
// Integer Definition
//=====================================================================
#ifndef __IINT8_DEFINED
#define __IINT8_DEFINED
typedef char IINT8;
#endif

#ifndef __IUINT8_DEFINED
#define __IUINT8_DEFINED
typedef unsigned char IUINT8;
#endif

#ifndef __IUINT16_DEFINED
#define __IUINT16_DEFINED
typedef unsigned short IUINT16;
#endif

#ifndef __IINT16_DEFINED
#define __IINT16_DEFINED
typedef short IINT16;
#endif

#ifndef __IINT32_DEFINED
#define __IINT32_DEFINED
typedef ISTDINT32 IINT32;
#endif

#ifndef __IUINT32_DEFINED
#define __IUINT32_DEFINED
typedef ISTDUINT32 IUINT32;
#endif

#ifndef __IINT64_DEFINED
#define __IINT64_DEFINED
#if defined(_MSC_VER) || defined(__BORLANDC__)
typedef __int64 IINT64;
#else
typedef long long IINT64;
#endif
#endif

#ifndef __IUINT64_DEFINED
#define __IUINT64_DEFINED
#if defined(_MSC_VER) || defined(__BORLANDC__)
typedef unsigned __int64 IUINT64;
#else
typedef unsigned long long IUINT64;
#endif
#endif

#ifndef INLINE
#if defined(__GNUC__)

#if (__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1))
#define INLINE         __inline__ __attribute__((always_inline))
#else
#define INLINE         __inline__
#endif

#elif (defined(_MSC_VER) || defined(__BORLANDC__) || defined(__WATCOMC__))
#define INLINE __inline
#else
#define INLINE 
#endif
#endif

#if (!defined(__cplusplus)) && (!defined(inline))
#define inline INLINE
#endif


//=====================================================================
// QUEUE DEFINITION                                                  
//=====================================================================
#ifndef __IQUEUE_DEF__
#define __IQUEUE_DEF__

struct IQUEUEHEAD {
	struct IQUEUEHEAD *next, *prev;
};

typedef struct IQUEUEHEAD iqueue_head;


//---------------------------------------------------------------------
// queue init                                                         
//---------------------------------------------------------------------
#define IQUEUE_HEAD_INIT(name) { &(name), &(name) }
#define IQUEUE_HEAD(name) \
	struct IQUEUEHEAD name = IQUEUE_HEAD_INIT(name)

#define IQUEUE_INIT(ptr) ( \
	(ptr)->next = (ptr), (ptr)->prev = (ptr))

#define IOFFSETOF(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#define ICONTAINEROF(ptr, type, member) ( \
		(type*)( ((char*)((type*)ptr)) - IOFFSETOF(type, member)) )

#define IQUEUE_ENTRY(ptr, type, member) ICONTAINEROF(ptr, type, member)


//---------------------------------------------------------------------
// queue operation                     
//---------------------------------------------------------------------
#define IQUEUE_ADD(node, head) ( \
	(node)->prev = (head), (node)->next = (head)->next, \
	(head)->next->prev = (node), (head)->next = (node))

#define IQUEUE_ADD_TAIL(node, head) ( \
	(node)->prev = (head)->prev, (node)->next = (head), \
	(head)->prev->next = (node), (head)->prev = (node))

#define IQUEUE_DEL_BETWEEN(p, n) ((n)->prev = (p), (p)->next = (n))

#define IQUEUE_DEL(entry) (\
	(entry)->next->prev = (entry)->prev, \
	(entry)->prev->next = (entry)->next, \
	(entry)->next = 0, (entry)->prev = 0)

#define IQUEUE_DEL_INIT(entry) do { \
	IQUEUE_DEL(entry); IQUEUE_INIT(entry); } while (0)

#define IQUEUE_IS_EMPTY(entry) ((entry) == (entry)->next)

#define iqueue_init		IQUEUE_INIT
#define iqueue_entry	IQUEUE_ENTRY
#define iqueue_add		IQUEUE_ADD
#define iqueue_add_tail	IQUEUE_ADD_TAIL
#define iqueue_del		IQUEUE_DEL
#define iqueue_del_init	IQUEUE_DEL_INIT
#define iqueue_is_empty IQUEUE_IS_EMPTY

#define IQUEUE_FOREACH(iterator, head, TYPE, MEMBER) \
	for ((iterator) = iqueue_entry((head)->next, TYPE, MEMBER); \
		&((iterator)->MEMBER) != (head); \
		(iterator) = iqueue_entry((iterator)->MEMBER.next, TYPE, MEMBER))

#define iqueue_foreach(iterator, head, TYPE, MEMBER) \
	IQUEUE_FOREACH(iterator, head, TYPE, MEMBER)

#define iqueue_foreach_entry(pos, head) \
	for( (pos) = (head)->next; (pos) != (head) ; (pos) = (pos)->next )


#define __iqueue_splice(list, head) do {	\
		iqueue_head *first = (list)->next, *last = (list)->prev; \
		iqueue_head *at = (head)->next; \
		(first)->prev = (head), (head)->next = (first);		\
		(last)->next = (at), (at)->prev = (last); }	while (0)

#define iqueue_splice(list, head) do { \
	if (!iqueue_is_empty(list)) __iqueue_splice(list, head); } while (0)

#define iqueue_splice_init(list, head) do {	\
	iqueue_splice(list, head);	iqueue_init(list); } while (0)


#ifdef _MSC_VER
#pragma warning(disable:4311)
#pragma warning(disable:4312)
#pragma warning(disable:4996)
#endif

#endif


//---------------------------------------------------------------------
// BYTE ORDER & ALIGNMENT
//---------------------------------------------------------------------
#ifndef IWORDS_BIG_ENDIAN
#ifdef _BIG_ENDIAN_
#if _BIG_ENDIAN_
#define IWORDS_BIG_ENDIAN 1
#endif
#endif
#ifndef IWORDS_BIG_ENDIAN
#if defined(__hppa__) || \
            defined(__m68k__) || defined(mc68000) || defined(_M_M68K) || \
            (defined(__MIPS__) && defined(__MIPSEB__)) || \
            defined(__ppc__) || defined(__POWERPC__) || defined(_M_PPC) || \
            defined(__sparc__) || defined(__powerpc__) || \
            defined(__mc68000__) || defined(__s390x__) || defined(__s390__)
#define IWORDS_BIG_ENDIAN 1
#endif
#endif
#ifndef IWORDS_BIG_ENDIAN
#define IWORDS_BIG_ENDIAN  0
#endif
#endif

#ifndef IWORDS_MUST_ALIGN
#if defined(__i386__) || defined(__i386) || defined(_i386_)
#define IWORDS_MUST_ALIGN 0
#elif defined(_M_IX86) || defined(_X86_) || defined(__x86_64__)
#define IWORDS_MUST_ALIGN 0
#elif defined(__amd64) || defined(__amd64__)
#define IWORDS_MUST_ALIGN 0
#else
#define IWORDS_MUST_ALIGN 1
#endif
#endif


//=====================================================================
// SEGMENT
//=====================================================================
struct IRCPSEG
{
	struct IQUEUEHEAD node;
	IUINT32 conv;
	IUINT32 cmd;
	IUINT32 frg;
	IUINT32 wnd;
	IUINT32 ts;
	IUINT32 sn;
	IUINT32 una;
	IUINT32 len;
	IUINT32 resendts; // 下次重传时间戳
	IUINT32 rto;
	IUINT32 fastack;
	IUINT32 xmit; // 发送次数
	char data[1];
	char rdata[1]; // 冗余数据, "[序号 长度 数据]"，"[IUINT32 IUINT32 DATA-LENGTH]" 
};


//---------------------------------------------------------------------
// IRCPCB
//---------------------------------------------------------------------
struct IRCPCB
{
	// conv会话编号, mtu最大分片大小, mss最大分片的data大小(即mtu减去首部大小), state为-1表示重传次数达到预警值(其实没其他用处)
	IUINT32 conv, mtu, mss, state;
	// snd_una未被确认的最小发送seq, 在snd_buf中的都是未被确认的, 所以snd_una总是等于snd_buf中第一个seg的seq或者为snd_nxt(snd_buf为空时)
	// snd_nxt为下次送到snd_buf的seq, 当seg从snd_queue移到snd_buf时, snd_nxt++
	// rcv_nxt为rcv_buf中下一个待接收的seq(这里待接收表示从rcv_buf到rcv_queue), 当seg从rcv_buf移到rcv_queue时rcv_nxt++
	IUINT32 snd_una, snd_nxt, rcv_nxt;
	// ssthresh拥塞窗口阈值
	IUINT32 ts_recent, ts_lastack, ssthresh;
	// rx_srtt往返时间估计值, rx_rto超时估计值, rx_rttval为计算rx_srtt和rx_rto的迭代值
	IINT32 rx_rttval, rx_srtt, rx_rto, rx_minrto;
	// 发送窗口大小, 接收窗口大小, 对端接收窗口大小, 拥塞窗口大小, 探测标记
	IUINT32 snd_wnd, rcv_wnd, rmt_wnd, cwnd, probe;
	// 当前时间戳, flush间隔, 下次flush时间戳, 发生超时重传的次数
	IUINT32 current, interval, ts_flush, xmit;
	// rcv_buf大小, snd_buf大小
	IUINT32 nrcv_buf, nsnd_buf;
	// rcv_que大小, snd_que大小
	IUINT32 nrcv_que, nsnd_que;
	IUINT32 nodelay, updated;
	// ts_probe下次探测时间, probe_wait探测间隔
	IUINT32 ts_probe, probe_wait;
	// dead_link为seg的重传预警值(其实没啥用), incr拥塞窗口最大分片限制, 决定拥塞窗口的大小
	IUINT32 dead_link, incr;
	struct IQUEUEHEAD snd_queue;
	struct IQUEUEHEAD rcv_queue;
	struct IQUEUEHEAD snd_buf;
	struct IQUEUEHEAD rcv_buf;
	IUINT32 *acklist; // 对对端的ack, (sn, ts)数组
	IUINT32 ackcount; // acklist.size/2
	IUINT32 ackblock; // 当ackcount达到ackblock时重新分配块2倍大小的acklist
	void *user; // 自定义数据
	char *buffer;
	int fastresend;
	int fastlimit;
	int nocwnd, stream;
	int logmask;
	int max_redu; // 最大冗余次数, max_redu为0即退化为kcp
	int(*output)(const char *buf, int len, struct IRCPCB *rcp, void *user);
	void(*writelog)(const char *log, struct IRCPCB *rcp, void *user);
};


typedef struct IRCPCB ircpcb;

#define IRCP_LOG_OUTPUT			1
#define IRCP_LOG_INPUT			2
#define IRCP_LOG_SEND			4
#define IRCP_LOG_RECV			8
#define IRCP_LOG_IN_DATA		16
#define IRCP_LOG_IN_ACK			32
#define IRCP_LOG_IN_PROBE		64
#define IRCP_LOG_IN_WINS		128
#define IRCP_LOG_OUT_DATA		256
#define IRCP_LOG_OUT_ACK		512
#define IRCP_LOG_OUT_PROBE		1024
#define IRCP_LOG_OUT_WINS		2048

#ifdef __cplusplus
extern "C" {
#endif

	//---------------------------------------------------------------------
	// interface
	//---------------------------------------------------------------------

	// create a new rcp control object, 'conv' must equal in two endpoint
	// from the same connection. 'user' will be passed to the output callback
	// output callback can be setup like this: 'rcp->output = my_udp_output'
	ircpcb* ircp_create(IUINT32 conv, void *user);

	// release rcp control object
	void ircp_release(ircpcb *rcp);

	// set output callback, which will be invoked by rcp
	void ircp_setoutput(ircpcb *rcp, int(*output)(const char *buf, int len,
		ircpcb *rcp, void *user));

	// user/upper level recv: returns size, returns below zero for EAGAIN
	int ircp_recv(ircpcb *rcp, char *buffer, int len);

	// user/upper level send, returns below zero for error
	int ircp_send(ircpcb *rcp, const char *buffer, int len);

	// update state (call it repeatedly, every 10ms-100ms), or you can ask 
	// ircp_check when to call it again (without ircp_input/_send calling).
	// 'current' - current timestamp in millisec. 
	void ircp_update(ircpcb *rcp, IUINT32 current);

	// Determine when should you invoke ircp_update:
	// returns when you should invoke ircp_update in millisec, if there 
	// is no ircp_input/_send calling. you can call ircp_update in that
	// time, instead of call update repeatly.
	// Important to reduce unnacessary ircp_update invoking. use it to 
	// schedule ircp_update (eg. implementing an epoll-like mechanism, 
	// or optimize ircp_update when handling massive rcp connections)
	IUINT32 ircp_check(const ircpcb *rcp, IUINT32 current);

	// when you received a low level packet (eg. UDP packet), call it
	int ircp_input(ircpcb *rcp, const char *data, long size);

	// flush pending data
	void ircp_flush(ircpcb *rcp);

	// check the size of next message in the recv queue
	int ircp_peeksize(const ircpcb *rcp);

	// change MTU size, default is 1400
	int ircp_setmtu(ircpcb *rcp, int mtu);

	// set maximum window size: sndwnd=32, rcvwnd=32 by default
	int ircp_wndsize(ircpcb *rcp, int sndwnd, int rcvwnd);

	// get how many packet is waiting to be sent
	int ircp_waitsnd(const ircpcb *rcp);

	// fastest: ircp_nodelay(rcp, 1, 20, 2, 1)
	// nodelay: 0:disable(default), 1:enable
	// interval: internal update timer interval in millisec, default is 100ms 
	// resend: 0:disable fast resend(default), 1:enable fast resend
	// nc: 0:normal congestion control(default), 1:disable congestion control
	// rd: max_redu
	int ircp_nodelay(ircpcb *rcp, int nodelay, int interval, int resend, int nc, int rd);


	void ircp_log(ircpcb *rcp, int mask, const char *fmt, ...);

	// setup allocator
	void ircp_allocator(void* (*new_malloc)(size_t), void(*new_free)(void*));

	// read conv
	IUINT32 ircp_getconv(const void *ptr);


#ifdef __cplusplus
}
#endif


#endif