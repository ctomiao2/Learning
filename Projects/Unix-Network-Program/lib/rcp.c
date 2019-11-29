//=====================================================================
//
// RCP - A Better ARQ Protocol Implementation
// skywind3000 (at) gmail.com, 2010-2011
//  
// Features:
// + Average RTT reduce 30% - 40% vs traditional ARQ like tcp.
// + Maximum RTT reduce three times vs tcp.
// + Lightweight, distributed as a single source file.
//
//=====================================================================
#include "rcp.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>



//=====================================================================
// RCP BASIC
//=====================================================================
const IUINT32 IRCP_RTO_NDL = 30;		// no delay min rto
const IUINT32 IRCP_RTO_MIN = 100;		// normal min rto
const IUINT32 IRCP_RTO_DEF = 200;
const IUINT32 IRCP_RTO_MAX = 60000;
const IUINT32 IRCP_CMD_PUSH = 81;		// cmd: push data
const IUINT32 IRCP_CMD_ACK = 82;		// cmd: ack
const IUINT32 IRCP_CMD_WASK = 83;		// cmd: window probe (ask)
const IUINT32 IRCP_CMD_WINS = 84;		// cmd: window size (tell)
const IUINT32 IRCP_ASK_SEND = 1;		// need to send IRCP_CMD_WASK
const IUINT32 IRCP_ASK_TELL = 2;		// need to send IRCP_CMD_WINS
const IUINT32 IRCP_WND_SND = 32;
const IUINT32 IRCP_WND_RCV = 128;       // must >= max fragment size
const IUINT32 IRCP_MTU_DEF = 1400;
const IUINT32 IRCP_ACK_FAST = 3;
const IUINT32 IRCP_INTERVAL = 100;
const IUINT32 IRCP_OVERHEAD = 24;
const IUINT32 IRCP_DEADLINK = 20;
const IUINT32 IRCP_THRESH_INIT = 2;
const IUINT32 IRCP_THRESH_MIN = 2;
const IUINT32 IRCP_PROBE_INIT = 7000;		// 7 secs to probe window size
const IUINT32 IRCP_PROBE_LIMIT = 120000;	// up to 120 secs to probe window
const IUINT32 IRCP_FASTACK_LIMIT = 5;		// max times to trigger fastack
const IUINT8  IRCP_MAX_REDUNDANCE = 1;


											//---------------------------------------------------------------------
											// encode / decode
											//---------------------------------------------------------------------

											/* encode 8 bits unsigned int */
static inline char *ircp_encode8u(char *p, unsigned char c)
{
	*(unsigned char*)p++ = c;
	return p;
}

/* decode 8 bits unsigned int */
static inline const char *ircp_decode8u(const char *p, unsigned char *c)
{
	*c = *(unsigned char*)p++;
	return p;
}

/* encode 16 bits unsigned int (lsb) */
static inline char *ircp_encode16u(char *p, unsigned short w)
{
#if IWORDS_BIG_ENDIAN || IWORDS_MUST_ALIGN
	*(unsigned char*)(p + 0) = (w & 255);
	*(unsigned char*)(p + 1) = (w >> 8);
#else
	*(unsigned short*)(p) = w;
#endif
	p += 2;
	return p;
}

/* decode 16 bits unsigned int (lsb) */
static inline const char *ircp_decode16u(const char *p, unsigned short *w)
{
#if IWORDS_BIG_ENDIAN || IWORDS_MUST_ALIGN
	*w = *(const unsigned char*)(p + 1);
	*w = *(const unsigned char*)(p + 0) + (*w << 8);
#else
	*w = *(const unsigned short*)p;
#endif
	p += 2;
	return p;
}

/* encode 32 bits unsigned int (lsb) */
static inline char *ircp_encode32u(char *p, IUINT32 l)
{
#if IWORDS_BIG_ENDIAN || IWORDS_MUST_ALIGN
	*(unsigned char*)(p + 0) = (unsigned char)((l >> 0) & 0xff);
	*(unsigned char*)(p + 1) = (unsigned char)((l >> 8) & 0xff);
	*(unsigned char*)(p + 2) = (unsigned char)((l >> 16) & 0xff);
	*(unsigned char*)(p + 3) = (unsigned char)((l >> 24) & 0xff);
#else
	*(IUINT32*)p = l;
#endif
	p += 4;
	return p;
}

/* decode 32 bits unsigned int (lsb) */
static inline const char *ircp_decode32u(const char *p, IUINT32 *l)
{
#if IWORDS_BIG_ENDIAN || IWORDS_MUST_ALIGN
	*l = *(const unsigned char*)(p + 3);
	*l = *(const unsigned char*)(p + 2) + (*l << 8);
	*l = *(const unsigned char*)(p + 1) + (*l << 8);
	*l = *(const unsigned char*)(p + 0) + (*l << 8);
#else 
	*l = *(const IUINT32*)p;
#endif
	p += 4;
	return p;
}

static inline IUINT32 _imin_(IUINT32 a, IUINT32 b) {
	return a <= b ? a : b;
}

static inline IUINT32 _imax_(IUINT32 a, IUINT32 b) {
	return a >= b ? a : b;
}

static inline IUINT32 _ibound_(IUINT32 lower, IUINT32 middle, IUINT32 upper)
{
	return _imin_(_imax_(lower, middle), upper);
}

static inline long _itimediff(IUINT32 later, IUINT32 earlier)
{
	return ((IINT32)(later - earlier));
}

//---------------------------------------------------------------------
// manage segment
//---------------------------------------------------------------------
typedef struct IRCPSEG IRCPSEG;

static void* (*ircp_malloc_hook)(size_t) = NULL;
static void(*ircp_free_hook)(void *) = NULL;

// internal malloc
static void* ircp_malloc(size_t size) {
	if (ircp_malloc_hook)
		return ircp_malloc_hook(size);
	return malloc(size);
}

// internal free
static void ircp_free(void *ptr) {
	if (ircp_free_hook) {
		ircp_free_hook(ptr);
	}
	else {
		free(ptr);
	}
}

// redefine allocator
void ircp_allocator(void* (*new_malloc)(size_t), void(*new_free)(void*))
{
	ircp_malloc_hook = new_malloc;
	ircp_free_hook = new_free;
}

// allocate a new rcp segment
static IRCPSEG* ircp_segment_new(ircpcb *rcp, int size)
{
	return (IRCPSEG*)ircp_malloc(sizeof(IRCPSEG) + size);
}

// delete a segment
static void ircp_segment_delete(ircpcb *rcp, IRCPSEG *seg)
{
	ircp_free(seg);
}

// write log
void ircp_log(ircpcb *rcp, int mask, const char *fmt, ...)
{
	char buffer[1024];
	va_list argptr;
	if ((mask & rcp->logmask) == 0 || rcp->writelog == 0) return;
	va_start(argptr, fmt);
	vsprintf(buffer, fmt, argptr);
	va_end(argptr);
	rcp->writelog(buffer, rcp, rcp->user);
}

// check log mask
static int ircp_canlog(const ircpcb *rcp, int mask)
{
	if ((mask & rcp->logmask) == 0 || rcp->writelog == NULL) return 0;
	return 1;
}

// output segment
static int ircp_output(ircpcb *rcp, const void *data, int size)
{
	assert(rcp);
	assert(rcp->output);
	if (ircp_canlog(rcp, IRCP_LOG_OUTPUT)) {
		ircp_log(rcp, IRCP_LOG_OUTPUT, "[RO] %ld bytes", (long)size);
	}
	if (size == 0) return 0;
	return rcp->output((const char*)data, size, rcp, rcp->user);
}

// output queue
void ircp_qprint(const char *name, const struct IQUEUEHEAD *head)
{
#if 0
	const struct IQUEUEHEAD *p;
	printf("<%s>: [", name);
	for (p = head->next; p != head; p = p->next) {
		const IRCPSEG *seg = iqueue_entry(p, const IRCPSEG, node);
		printf("(%lu %d)", (unsigned long)seg->sn, (int)(seg->ts % 10000));
		if (p->next != head) printf(",");
	}
	printf("]\n");
#endif
}


//---------------------------------------------------------------------
// create a new rcpcb
//---------------------------------------------------------------------
ircpcb* ircp_create(IUINT32 conv, void *user)
{
	ircpcb *rcp = (ircpcb*)ircp_malloc(sizeof(struct IRCPCB));
	if (rcp == NULL) return NULL;
	rcp->conv = conv;
	rcp->user = user;
	rcp->snd_una = 0;
	rcp->snd_nxt = 0;
	rcp->rcv_nxt = 0;
	rcp->ts_recent = 0;
	rcp->ts_lastack = 0;
	rcp->ts_probe = 0;
	rcp->probe_wait = 0;
	rcp->snd_wnd = IRCP_WND_SND;
	rcp->rcv_wnd = IRCP_WND_RCV;
	rcp->rmt_wnd = IRCP_WND_RCV;
	rcp->cwnd = 0;
	rcp->incr = 0;
	rcp->probe = 0;
	rcp->mtu = IRCP_MTU_DEF;
	rcp->mss = rcp->mtu - IRCP_OVERHEAD;
	rcp->stream = 0;

	rcp->buffer = (char*)ircp_malloc((rcp->mtu + IRCP_OVERHEAD) * 3);
	if (rcp->buffer == NULL) {
		ircp_free(rcp);
		return NULL;
	}

	iqueue_init(&rcp->snd_queue);
	iqueue_init(&rcp->rcv_queue);
	iqueue_init(&rcp->snd_buf);
	iqueue_init(&rcp->rcv_buf);
	rcp->nrcv_buf = 0;
	rcp->nsnd_buf = 0;
	rcp->nrcv_que = 0;
	rcp->nsnd_que = 0;
	rcp->state = 0;
	rcp->acklist = NULL;
	rcp->ackblock = 0;
	rcp->ackcount = 0;
	rcp->rx_srtt = 0;
	rcp->rx_rttval = 0;
	rcp->rx_rto = IRCP_RTO_DEF;
	rcp->rx_minrto = IRCP_RTO_MIN;
	rcp->current = 0;
	rcp->interval = IRCP_INTERVAL;
	rcp->ts_flush = IRCP_INTERVAL;
	rcp->nodelay = 0;
	rcp->updated = 0;
	rcp->logmask = 0;
	rcp->ssthresh = IRCP_THRESH_INIT;
	rcp->fastresend = 0;
	rcp->fastlimit = IRCP_FASTACK_LIMIT;
	rcp->nocwnd = 0;
	rcp->xmit = 0;
	rcp->dead_link = IRCP_DEADLINK;
	rcp->output = NULL;
	rcp->writelog = NULL;

	return rcp;
}


//---------------------------------------------------------------------
// release a new rcpcb
//---------------------------------------------------------------------
void ircp_release(ircpcb *rcp)
{
	assert(rcp);
	if (rcp) {
		IRCPSEG *seg;
		while (!iqueue_is_empty(&rcp->snd_buf)) {
			seg = iqueue_entry(rcp->snd_buf.next, IRCPSEG, node);
			iqueue_del(&seg->node);
			ircp_segment_delete(rcp, seg);
		}
		while (!iqueue_is_empty(&rcp->rcv_buf)) {
			seg = iqueue_entry(rcp->rcv_buf.next, IRCPSEG, node);
			iqueue_del(&seg->node);
			ircp_segment_delete(rcp, seg);
		}
		while (!iqueue_is_empty(&rcp->snd_queue)) {
			seg = iqueue_entry(rcp->snd_queue.next, IRCPSEG, node);
			iqueue_del(&seg->node);
			ircp_segment_delete(rcp, seg);
		}
		while (!iqueue_is_empty(&rcp->rcv_queue)) {
			seg = iqueue_entry(rcp->rcv_queue.next, IRCPSEG, node);
			iqueue_del(&seg->node);
			ircp_segment_delete(rcp, seg);
		}
		if (rcp->buffer) {
			ircp_free(rcp->buffer);
		}
		if (rcp->acklist) {
			ircp_free(rcp->acklist);
		}

		rcp->nrcv_buf = 0;
		rcp->nsnd_buf = 0;
		rcp->nrcv_que = 0;
		rcp->nsnd_que = 0;
		rcp->ackcount = 0;
		rcp->buffer = NULL;
		rcp->acklist = NULL;
		ircp_free(rcp);
	}
}


//---------------------------------------------------------------------
// set output callback, which will be invoked by rcp
//---------------------------------------------------------------------
void ircp_setoutput(ircpcb *rcp, int(*output)(const char *buf, int len,
	ircpcb *rcp, void *user))
{
	rcp->output = output;
}


//---------------------------------------------------------------------
// user/upper level recv: returns size, returns below zero for EAGAIN
//---------------------------------------------------------------------
int ircp_recv(ircpcb *rcp, char *buffer, int len)
{
	struct IQUEUEHEAD *p;
	int ispeek = (len < 0) ? 1 : 0;
	int peeksize;
	int recover = 0;
	IRCPSEG *seg;
	assert(rcp);

	if (iqueue_is_empty(&rcp->rcv_queue))
		return -1;

	if (len < 0) len = -len;

	peeksize = ircp_peeksize(rcp);

	if (peeksize < 0)
		return -2;

	if (peeksize > len)
		return -3;

	if (rcp->nrcv_que >= rcp->rcv_wnd)
		recover = 1;

	// merge fragment
	for (len = 0, p = rcp->rcv_queue.next; p != &rcp->rcv_queue; ) {
		int fragment;
		seg = iqueue_entry(p, IRCPSEG, node);
		p = p->next;

		if (buffer) {
			memcpy(buffer, seg->data, seg->len);
			buffer += seg->len;
		}

		len += seg->len;
		fragment = seg->frg;

		if (ircp_canlog(rcp, IRCP_LOG_RECV)) {
			ircp_log(rcp, IRCP_LOG_RECV, "recv sn=%lu", seg->sn);
		}

		if (ispeek == 0) {
			iqueue_del(&seg->node);
			ircp_segment_delete(rcp, seg);
			rcp->nrcv_que--;
		}

		if (fragment == 0)
			break;
	}

	assert(len == peeksize);

	// move available data from rcv_buf -> rcv_queue
	while (!iqueue_is_empty(&rcp->rcv_buf)) {
		IRCPSEG *seg = iqueue_entry(rcp->rcv_buf.next, IRCPSEG, node);
		if (seg->sn == rcp->rcv_nxt && rcp->nrcv_que < rcp->rcv_wnd) {
			iqueue_del(&seg->node);
			rcp->nrcv_buf--;
			iqueue_add_tail(&seg->node, &rcp->rcv_queue);
			rcp->nrcv_que++;
			rcp->rcv_nxt++;
		}
		else {
			break;
		}
	}

	// fast recover
	if (rcp->nrcv_que < rcp->rcv_wnd && recover) {
		// ready to send back IRCP_CMD_WINS in ircp_flush
		// tell remote my window size
		rcp->probe |= IRCP_ASK_TELL;
	}

	return len;
}


//---------------------------------------------------------------------
// peek data size
//---------------------------------------------------------------------
int ircp_peeksize(const ircpcb *rcp)
{
	struct IQUEUEHEAD *p;
	IRCPSEG *seg;
	int length = 0;

	assert(rcp);

	if (iqueue_is_empty(&rcp->rcv_queue)) return -1;

	seg = iqueue_entry(rcp->rcv_queue.next, IRCPSEG, node);
	if (seg->frg == 0) return seg->len;

	if (rcp->nrcv_que < seg->frg + 1) return -1;

	for (p = rcp->rcv_queue.next; p != &rcp->rcv_queue; p = p->next) {
		seg = iqueue_entry(p, IRCPSEG, node);
		length += seg->len;
		if (seg->frg == 0) break;
	}

	return length;
}


//---------------------------------------------------------------------
// user/upper level send, returns below zero for error
// 仅将数据追加到rcp->snd_queue, 不更新rcp->snd_buf和rcp->snd_nxt
//---------------------------------------------------------------------
int ircp_send(ircpcb *rcp, const char *buffer, int len)
{
	IRCPSEG *seg;
	int count, i;

	assert(rcp->mss > 0);
	if (len < 0) return -1;

	// append to previous segment in streaming mode (if possible)
	if (rcp->stream != 0) {
		if (!iqueue_is_empty(&rcp->snd_queue)) {
			IRCPSEG *old = iqueue_entry(rcp->snd_queue.prev, IRCPSEG, node);
			if (old->len < rcp->mss) {
				int capacity = rcp->mss - old->len;
				int extend = (len < capacity) ? len : capacity;
				seg = ircp_segment_new(rcp, old->len + extend);
				assert(seg);
				if (seg == NULL) {
					return -2;
				}
				iqueue_add_tail(&seg->node, &rcp->snd_queue);
				memcpy(seg->data, old->data, old->len);
				if (buffer) {
					memcpy(seg->data + old->len, buffer, extend);
					buffer += extend;
				}
				seg->len = old->len + extend;
				seg->frg = 0;
				len -= extend;
				iqueue_del_init(&old->node);
				ircp_segment_delete(rcp, old);
			}
		}
		if (len <= 0) {
			return 0;
		}
	}

	if (len <= (int)rcp->mss) count = 1;
	else count = (len + rcp->mss - 1) / rcp->mss;

	if (count >= (int)IRCP_WND_RCV) return -2;

	if (count == 0) count = 1;

	// fragment
	for (i = 0; i < count; i++) {
		int size = len >(int)rcp->mss ? (int)rcp->mss : len;
		seg = ircp_segment_new(rcp, size);
		assert(seg);
		if (seg == NULL) {
			return -2;
		}
		if (buffer && len > 0) {
			memcpy(seg->data, buffer, size);
		}
		seg->len = size;
		seg->frg = (rcp->stream == 0) ? (count - i - 1) : 0;
		iqueue_init(&seg->node);
		iqueue_add_tail(&seg->node, &rcp->snd_queue);
		rcp->nsnd_que++;
		if (buffer) {
			buffer += size;
		}
		len -= size;
	}

	return 0;
}


//---------------------------------------------------------------------
// parse ack, 计算RTT(往返估算时间)和RTO(超时估算时间), 采用Jacobaon/Karels算法(RFC6298)
// 算法实现：
// 初始值: 
//		srtt = r (r是第一个seq的真实rtt), rttval = r/2, rto = srtt + max(G, K*rttvak)
// 在下面算法中G取rcp->interval, K取4
// 后续迭代：
//		rttval = (1-beta)*rttval + beta*|srtt - r| (r为当前seq的真实rtt值)
//      srtt =  (1-alpha)*srtt + alpha*r
//      rto = srtt + max(G, K*rttval)
// 这里alphs = 1/8, beta = 1/4
//---------------------------------------------------------------------
static void ircp_update_ack(ircpcb *rcp, IINT32 rtt)
{
	IINT32 rto = 0;
	if (rcp->rx_srtt == 0) {
		rcp->rx_srtt = rtt;
		rcp->rx_rttval = rtt / 2;
	}
	else {
		long delta = rtt - rcp->rx_srtt;
		if (delta < 0) delta = -delta;
		rcp->rx_rttval = (3 * rcp->rx_rttval + delta) / 4;
		rcp->rx_srtt = (7 * rcp->rx_srtt + rtt) / 8;
		if (rcp->rx_srtt < 1) rcp->rx_srtt = 1;
	}
	rto = rcp->rx_srtt + _imax_(rcp->interval, 4 * rcp->rx_rttval);
	rcp->rx_rto = _ibound_(rcp->rx_minrto, rto, IRCP_RTO_MAX);
}

static void ircp_shrink_buf(ircpcb *rcp)
{
	struct IQUEUEHEAD *p = rcp->snd_buf.next;
	if (p != &rcp->snd_buf) {
		IRCPSEG *seg = iqueue_entry(p, IRCPSEG, node);
		rcp->snd_una = seg->sn;
	}
	else {
		rcp->snd_una = rcp->snd_nxt;
	}
}

// 对单个sn的确认, 只将sn从snd_buf移除
static void ircp_parse_ack(ircpcb *rcp, IUINT32 sn)
{
	struct IQUEUEHEAD *p, *next;

	if (_itimediff(sn, rcp->snd_una) < 0 || _itimediff(sn, rcp->snd_nxt) >= 0)
		return;

	for (p = rcp->snd_buf.next; p != &rcp->snd_buf; p = next) {
		IRCPSEG *seg = iqueue_entry(p, IRCPSEG, node);
		next = p->next;
		if (sn == seg->sn) {
			iqueue_del(p);
			ircp_segment_delete(rcp, seg);
			rcp->nsnd_buf--;
			break;
		}
		if (_itimediff(sn, seg->sn) < 0) {
			break;
		}
	}
}

// una之前的都已收到确认, 从snd_buf里移除
static void ircp_parse_una(ircpcb *rcp, IUINT32 una)
{
	struct IQUEUEHEAD *p, *next;
	for (p = rcp->snd_buf.next; p != &rcp->snd_buf; p = next) {
		IRCPSEG *seg = iqueue_entry(p, IRCPSEG, node);
		next = p->next;
		if (_itimediff(una, seg->sn) > 0) {
			iqueue_del(p);
			ircp_segment_delete(rcp, seg);
			rcp->nsnd_buf--;
		}
		else {
			break;
		}
	}
}

// IRCP_FASTACK_CONSERVE模式, snd_buf中在sn之前的seg的fastack都加1
// 非IRCP_FASTACK_CONSERVE模式, snd_buf中在ts及其之前的seg的fastack都加1
static void ircp_parse_fastack(ircpcb *rcp, IUINT32 sn, IUINT32 ts)
{
	struct IQUEUEHEAD *p, *next;
	// 在snd_una之前发送的都已被确认, snd_nxt后的还没开始发送
	if (_itimediff(sn, rcp->snd_una) < 0 || _itimediff(sn, rcp->snd_nxt) >= 0)
		return;

	for (p = rcp->snd_buf.next; p != &rcp->snd_buf; p = next) {
		IRCPSEG *seg = iqueue_entry(p, IRCPSEG, node);
		next = p->next;
		if (_itimediff(sn, seg->sn) < 0) {
			break;
		}
		else if (sn != seg->sn) {
#ifndef IRCP_FASTACK_CONSERVE
			seg->fastack++;
#else
			if (_itimediff(ts, seg->ts) >= 0)
				seg->fastack++;
#endif
		}
	}
}


//---------------------------------------------------------------------
// ack append
//---------------------------------------------------------------------
static void ircp_ack_push(ircpcb *rcp, IUINT32 sn, IUINT32 ts)
{
	size_t newsize = rcp->ackcount + 1;
	IUINT32 *ptr;

	if (newsize > rcp->ackblock) {
		IUINT32 *acklist;
		size_t newblock;

		for (newblock = 8; newblock < newsize; newblock <<= 1);
		acklist = (IUINT32*)ircp_malloc(newblock * sizeof(IUINT32) * 2);

		if (acklist == NULL) {
			assert(acklist != NULL);
			abort();
		}

		if (rcp->acklist != NULL) {
			size_t x;
			for (x = 0; x < rcp->ackcount; x++) {
				acklist[x * 2 + 0] = rcp->acklist[x * 2 + 0];
				acklist[x * 2 + 1] = rcp->acklist[x * 2 + 1];
			}
			ircp_free(rcp->acklist);
		}

		rcp->acklist = acklist;
		rcp->ackblock = newblock;
	}

	ptr = &rcp->acklist[rcp->ackcount * 2];
	ptr[0] = sn;
	ptr[1] = ts;
	rcp->ackcount++;
}

static void ircp_ack_get(const ircpcb *rcp, int p, IUINT32 *sn, IUINT32 *ts)
{
	if (sn) sn[0] = rcp->acklist[p * 2 + 0];
	if (ts) ts[0] = rcp->acklist[p * 2 + 1];
}


//---------------------------------------------------------------------
// parse data
//---------------------------------------------------------------------
void ircp_parse_data(ircpcb *rcp, IRCPSEG *newseg)
{
	struct IQUEUEHEAD *p, *prev;
	IUINT32 sn = newseg->sn;
	int repeat = 0;
	// 不在接收窗口内直接丢弃
	if (_itimediff(sn, rcp->rcv_nxt + rcp->rcv_wnd) >= 0 ||
		_itimediff(sn, rcp->rcv_nxt) < 0) {
		ircp_segment_delete(rcp, newseg);
		return;
	}

	for (p = rcp->rcv_buf.prev; p != &rcp->rcv_buf; p = prev) {
		IRCPSEG *seg = iqueue_entry(p, IRCPSEG, node);
		prev = p->prev;
		// 重复接收
		if (seg->sn == sn) {
			repeat = 1;
			break;
		}
		if (_itimediff(sn, seg->sn) > 0) {
			break;
		}
	}
	// 按顺序插入到rcp->rcv_buf中去
	if (repeat == 0) {
		iqueue_init(&newseg->node);
		iqueue_add(&newseg->node, p);
		rcp->nrcv_buf++;
	}
	else {
		ircp_segment_delete(rcp, newseg);
	}

#if 0
	ircp_qprint("rcvbuf", &rcp->rcv_buf);
	printf("rcv_nxt=%lu\n", rcp->rcv_nxt);
#endif

	// move available data from rcv_buf -> rcv_queue
	while (!iqueue_is_empty(&rcp->rcv_buf)) {
		IRCPSEG *seg = iqueue_entry(rcp->rcv_buf.next, IRCPSEG, node);
		if (seg->sn == rcp->rcv_nxt && rcp->nrcv_que < rcp->rcv_wnd) {
			iqueue_del(&seg->node);
			rcp->nrcv_buf--;
			iqueue_add_tail(&seg->node, &rcp->rcv_queue);
			rcp->nrcv_que++;
			rcp->rcv_nxt++;
		}
		else {
			break;
		}
	}

#if 0
	ircp_qprint("queue", &rcp->rcv_queue);
	printf("rcv_nxt=%lu\n", rcp->rcv_nxt);
#endif

#if 1
	//	printf("snd(buf=%d, queue=%d)\n", rcp->nsnd_buf, rcp->nsnd_que);
	//	printf("rcv(buf=%d, queue=%d)\n", rcp->nrcv_buf, rcp->nrcv_que);
#endif
}


//---------------------------------------------------------------------
// input data
//---------------------------------------------------------------------
int ircp_input(ircpcb *rcp, const char *data, long size)
{
	IUINT32 prev_una = rcp->snd_una;
	IUINT32 maxack = 0, latest_ts = 0;
	int flag = 0;

	if (ircp_canlog(rcp, IRCP_LOG_INPUT)) {
		ircp_log(rcp, IRCP_LOG_INPUT, "[RI] %d bytes", size);
	}

	if (data == NULL || (int)size < (int)IRCP_OVERHEAD) return -1;

	while (1) {
		IUINT32 ts, sn, len, una, conv;
		IUINT16 wnd;
		IUINT8 cmd, frg;
		IRCPSEG *seg;

		if (size < (int)IRCP_OVERHEAD) break;

		data = ircp_decode32u(data, &conv);
		if (conv != rcp->conv) return -1;

		data = ircp_decode8u(data, &cmd);
		data = ircp_decode8u(data, &frg);
		data = ircp_decode16u(data, &wnd);
		data = ircp_decode32u(data, &ts);
		data = ircp_decode32u(data, &sn);
		data = ircp_decode32u(data, &una);
		data = ircp_decode32u(data, &len);

		size -= IRCP_OVERHEAD;

		if ((long)size < (long)len || (int)len < 0) return -2;

		if (cmd != IRCP_CMD_PUSH && cmd != IRCP_CMD_ACK &&
			cmd != IRCP_CMD_WASK && cmd != IRCP_CMD_WINS)
			return -3;
		// 对端发送的任何类型数据都会带上自己的拥塞窗口大小
		rcp->rmt_wnd = wnd;
		// 将una之前的seg都从snd_buf移除(不包括una)
		ircp_parse_una(rcp, una);
		// 更新rcp->snd_una
		ircp_shrink_buf(rcp);

		// 是对之前发送的确认
		if (cmd == IRCP_CMD_ACK) {
			if (_itimediff(rcp->current, ts) >= 0) {
				// 更新rtt和rto
				ircp_update_ack(rcp, _itimediff(rcp->current, ts));
			}
			// 将sn从snd_buf中移除
			ircp_parse_ack(rcp, sn);
			// 更新rcp->snd_una(snd_una为第一个未确认的seg)
			ircp_shrink_buf(rcp);
			if (flag == 0) {
				flag = 1;
				maxack = sn;
				latest_ts = ts;
			}
			else {
				if (_itimediff(sn, maxack) > 0) {
#ifndef IRCP_FASTACK_CONSERVE
					maxack = sn;
					latest_ts = ts;
#else
					if (_itimediff(ts, latest_ts) > 0) {
						maxack = sn;
						latest_ts = ts;
					}
#endif
				}
			}
			if (ircp_canlog(rcp, IRCP_LOG_IN_ACK)) {
				ircp_log(rcp, IRCP_LOG_IN_DATA,
					"input ack: sn=%lu rtt=%ld rto=%ld", sn,
					(long)_itimediff(rcp->current, ts),
					(long)rcp->rx_rto);
			}
		}

		// 对端发送数据过来
		else if (cmd == IRCP_CMD_PUSH) {
			if (ircp_canlog(rcp, IRCP_LOG_IN_DATA)) {
				ircp_log(rcp, IRCP_LOG_IN_DATA,
					"input psh: sn=%lu ts=%lu", sn, ts);
			}
			// sn在接收窗口内, rcp->rcv_nxt之前的seg都已递交给上层
			if (_itimediff(sn, rcp->rcv_nxt + rcp->rcv_wnd) < 0) {
				// 将sn,ts加入到rcp->acklist中去, 重复收到的也记上
				ircp_ack_push(rcp, sn, ts);
				// sn此前尚未接收到
				if (_itimediff(sn, rcp->rcv_nxt) >= 0) {
					seg = ircp_segment_new(rcp, len);
					seg->conv = conv;
					seg->cmd = cmd;
					seg->frg = frg;
					seg->wnd = wnd;
					seg->ts = ts;
					seg->sn = sn;
					seg->una = una;
					seg->len = len;

					if (len > 0) {
						memcpy(seg->data, data, len);
					}
					// 将seg插入到rcp->rcv_buf中去, 并试图从rcp->rcv_nxt开始的连续段从rcp->rcv_buf移到rcp->rcv_queue
					ircp_parse_data(rcp, seg);
				}
			}
		}
		// 对端询问窗口大小
		else if (cmd == IRCP_CMD_WASK) {
			// ready to send back IRCP_CMD_WINS in ircp_flush
			// tell remote my window size
			rcp->probe |= IRCP_ASK_TELL;
			if (ircp_canlog(rcp, IRCP_LOG_IN_PROBE)) {
				ircp_log(rcp, IRCP_LOG_IN_PROBE, "input probe");
			}
		}
		// 对端回复自己的窗口大小
		else if (cmd == IRCP_CMD_WINS) {
			// do nothing
			if (ircp_canlog(rcp, IRCP_LOG_IN_WINS)) {
				ircp_log(rcp, IRCP_LOG_IN_WINS,
					"input wins: %lu", (IUINT32)(wnd));
			}
		}
		else {
			return -3;
		}

		data += len;
		size -= len;
	}

	if (flag != 0) {
		// 将rcp->snd_buf中maxack或latest_ts之前的seg->fastack++
		ircp_parse_fastack(rcp, maxack, latest_ts);
	}

	// snd_una有更新, 拥塞窗口大小也更新(与tcp不同的是这里并不是无脑+1)
	if (_itimediff(rcp->snd_una, prev_una) > 0) {
		if (rcp->cwnd < rcp->rmt_wnd) {
			IUINT32 mss = rcp->mss;
			if (rcp->cwnd < rcp->ssthresh) {
				rcp->cwnd++;
				rcp->incr += mss; // 最大发送数据片大小
			}
			else {
				if (rcp->incr < mss) rcp->incr = mss;
				// rcp->incr取最小mss时增幅最大, 为mss + mss/16, 当rcp->incr越大其增幅越小, 但不会低于mss/16
				// 增幅为对数函数走势
				rcp->incr += (mss * mss) / rcp->incr + (mss / 16);
				if ((rcp->cwnd + 1) * mss <= rcp->incr) {
					rcp->cwnd++;
				}
			}
			//不能大于对端的接收窗口大小
			if (rcp->cwnd > rcp->rmt_wnd) {
				rcp->cwnd = rcp->rmt_wnd;
				rcp->incr = rcp->rmt_wnd * mss;
			}
		}
	}

	return 0;
}


//---------------------------------------------------------------------
// ircp_encode_seg
//---------------------------------------------------------------------
static char *ircp_encode_seg(char *ptr, const IRCPSEG *seg)
{
	ptr = ircp_encode32u(ptr, seg->conv);
	ptr = ircp_encode8u(ptr, (IUINT8)seg->cmd);
	ptr = ircp_encode8u(ptr, (IUINT8)seg->frg);
	ptr = ircp_encode16u(ptr, (IUINT16)seg->wnd);
	ptr = ircp_encode32u(ptr, seg->ts);
	ptr = ircp_encode32u(ptr, seg->sn);
	ptr = ircp_encode32u(ptr, seg->una);
	ptr = ircp_encode32u(ptr, seg->len);
	return ptr;
}

static int ircp_wnd_unused(const ircpcb *rcp)
{
	if (rcp->nrcv_que < rcp->rcv_wnd) {
		return rcp->rcv_wnd - rcp->nrcv_que;
	}
	return 0;
}


//---------------------------------------------------------------------
// ircp_flush
//---------------------------------------------------------------------
void ircp_flush(ircpcb *rcp)
{
	IUINT32 current = rcp->current;
	// 用来保存待发送的数据分片
	char *buffer = rcp->buffer;
	char *ptr = buffer;
	int count, size, i;
	IUINT32 resent, cwnd;
	IUINT32 rtomin;
	struct IQUEUEHEAD *p;
	int change = 0;
	int lost = 0;
	IRCPSEG seg;

	// 'ircp_update' haven't been called. 
	if (rcp->updated == 0) return;

	seg.conv = rcp->conv;
	seg.cmd = IRCP_CMD_ACK;
	seg.frg = 0;
	// 己方接收窗口大小(最大接收窗口 - rcv_queue大小)
	seg.wnd = ircp_wnd_unused(rcp);
	// 下一个待接收的seq是rcp->rcv_nxt
	seg.una = rcp->rcv_nxt;
	seg.len = 0;
	seg.sn = 0;
	seg.ts = 0;

	// flush acknowledges, 对对端所有发送过来的seg都要逐一确认(无论是否重复收到)
	count = rcp->ackcount;
	for (i = 0; i < count; i++) {
		size = (int)(ptr - buffer);
		if (size + (int)IRCP_OVERHEAD >(int)rcp->mtu) {
			ircp_output(rcp, buffer, size);
			ptr = buffer;
		}
		// 从rcp->acklist获取收到的第i个seg的sn和ts
		ircp_ack_get(rcp, i, &seg.sn, &seg.ts);
		// 编码保存到buffer, 等达到最大分片mtu时递交给上层发送给对端
		ptr = ircp_encode_seg(ptr, &seg);
	}
	// 所有待确认的seg都已发送确认完毕
	rcp->ackcount = 0;

	// probe window size (if remote window size equals zero)
	if (rcp->rmt_wnd == 0) {
		if (rcp->probe_wait == 0) {
			rcp->probe_wait = IRCP_PROBE_INIT;
			rcp->ts_probe = rcp->current + rcp->probe_wait;
		}
		else {
			if (_itimediff(rcp->current, rcp->ts_probe) >= 0) {
				if (rcp->probe_wait < IRCP_PROBE_INIT)
					rcp->probe_wait = IRCP_PROBE_INIT;
				rcp->probe_wait += rcp->probe_wait / 2;
				if (rcp->probe_wait > IRCP_PROBE_LIMIT)
					rcp->probe_wait = IRCP_PROBE_LIMIT;
				rcp->ts_probe = rcp->current + rcp->probe_wait;
				rcp->probe |= IRCP_ASK_SEND;
			}
		}
	}
	else {
		rcp->ts_probe = 0;
		rcp->probe_wait = 0;
	}

	// flush window probing commands
	if (rcp->probe & IRCP_ASK_SEND) {
		seg.cmd = IRCP_CMD_WASK;
		size = (int)(ptr - buffer);
		if (size + (int)IRCP_OVERHEAD > (int)rcp->mtu) {
			ircp_output(rcp, buffer, size);
			ptr = buffer;
		}
		ptr = ircp_encode_seg(ptr, &seg);
	}

	// flush window probing commands
	if (rcp->probe & IRCP_ASK_TELL) {
		seg.cmd = IRCP_CMD_WINS;
		size = (int)(ptr - buffer);
		if (size + (int)IRCP_OVERHEAD > (int)rcp->mtu) {
			ircp_output(rcp, buffer, size);
			ptr = buffer;
		}
		ptr = ircp_encode_seg(ptr, &seg);
	}

	rcp->probe = 0;

	// calculate window size
	cwnd = _imin_(rcp->snd_wnd, rcp->rmt_wnd);
	if (rcp->nocwnd == 0) cwnd = _imin_(rcp->cwnd, cwnd);

	// move data from snd_queue to snd_buf
	while (_itimediff(rcp->snd_nxt, rcp->snd_una + cwnd) < 0) {
		IRCPSEG *newseg;
		if (iqueue_is_empty(&rcp->snd_queue)) break;

		newseg = iqueue_entry(rcp->snd_queue.next, IRCPSEG, node);

		iqueue_del(&newseg->node);
		iqueue_add_tail(&newseg->node, &rcp->snd_buf);
		rcp->nsnd_que--;
		rcp->nsnd_buf++;

		newseg->conv = rcp->conv;
		newseg->cmd = IRCP_CMD_PUSH;
		newseg->wnd = seg.wnd;
		newseg->ts = current;
		newseg->sn = rcp->snd_nxt++;
		newseg->una = rcp->rcv_nxt;
		newseg->resendts = current;
		newseg->rto = rcp->rx_rto;
		newseg->fastack = 0;
		newseg->xmit = 0;
	}

	// calculate resent
	resent = (rcp->fastresend > 0) ? (IUINT32)rcp->fastresend : 0xffffffff;
	rtomin = (rcp->nodelay == 0) ? (rcp->rx_rto >> 3) : 0;

	// flush data segments
	for (p = rcp->snd_buf.next; p != &rcp->snd_buf; p = p->next) {
		IRCPSEG *segment = iqueue_entry(p, IRCPSEG, node);
		int needsend = 0;
		// 尚未发送过
		if (segment->xmit == 0) {
			needsend = 1;
			segment->xmit++;
			segment->rto = rcp->rx_rto;
			// 设置超时重传时间戳
			segment->resendts = current + segment->rto + rtomin;
		}
		// 超时重传
		else if (_itimediff(current, segment->resendts) >= 0) {
			needsend = 1;
			segment->xmit++;
			rcp->xmit++;
			// 发生超时则上调rto, 不开nodelay是2倍rto, 开了是1.5倍rto
			if (rcp->nodelay == 0) {
				segment->rto += rcp->rx_rto;
			}
			else {
				segment->rto += rcp->rx_rto / 2;
			}
			segment->resendts = current + segment->rto;
			lost = 1;
		}
		// 当收到一个超前的seg确认, 那么在seg之前的任何seg的fastack都会+1
		// 当fastack达到设定的快速重传阈值时就会立马重传该seg而不是去等超时重传
		// 如果设置了快速重传次数限制fastlimit, 那么当seg的重传次数达到fastlimit后快速重传机制就会失效, 这种情况只能依赖超时重传了
		else if (segment->fastack >= resent) {
			if ((int)segment->xmit <= rcp->fastlimit ||
				rcp->fastlimit <= 0) {
				needsend = 1;
				segment->xmit++;
				segment->fastack = 0;
				segment->resendts = current + segment->rto;
				change++;
			}
		}

		if (needsend) {
			int size, need;
			segment->ts = current;
			segment->wnd = seg.wnd;
			segment->una = rcp->rcv_nxt;

			size = (int)(ptr - buffer);
			need = IRCP_OVERHEAD + segment->len;

			if (size + need > (int)rcp->mtu) {
				ircp_output(rcp, buffer, size);
				ptr = buffer;
			}

			ptr = ircp_encode_seg(ptr, segment);

			if (segment->len > 0) {
				memcpy(ptr, segment->data, segment->len);
				ptr += segment->len;
			}
			// 发送次数达到dead_link, 其实没啥用, 只是标记下
			if (segment->xmit >= rcp->dead_link) {
				rcp->state = -1;
			}
		}
	}

	// flash remain segments
	size = (int)(ptr - buffer);
	if (size > 0) {
		ircp_output(rcp, buffer, size);
	}

	// update ssthresh
	// 已触发快速重传机制(尚未超时)
	if (change) {
		IUINT32 inflight = rcp->snd_nxt - rcp->snd_una;
		// 阈值设定为未确认的已发送分段数量的一半
		rcp->ssthresh = inflight / 2;
		if (rcp->ssthresh < IRCP_THRESH_MIN)
			rcp->ssthresh = IRCP_THRESH_MIN;
		// 使用快恢复算法
		rcp->cwnd = rcp->ssthresh + resent;
		rcp->incr = rcp->cwnd * rcp->mss;
	}
	// 发生超时了, 使用慢启动算法, 阈值为当前拥塞窗口大小的一半并将拥塞窗口大小下调到1
	if (lost) {
		rcp->ssthresh = cwnd / 2;
		if (rcp->ssthresh < IRCP_THRESH_MIN)
			rcp->ssthresh = IRCP_THRESH_MIN;
		rcp->cwnd = 1;
		rcp->incr = rcp->mss;
	}

	if (rcp->cwnd < 1) {
		rcp->cwnd = 1;
		rcp->incr = rcp->mss;
	}
}


//---------------------------------------------------------------------
// update state (call it repeatedly, every 10ms-100ms), or you can ask 
// ircp_check when to call it again (without ircp_input/_send calling).
// 'current' - current timestamp in millisec. 
//---------------------------------------------------------------------
void ircp_update(ircpcb *rcp, IUINT32 current)
{
	IINT32 slap;

	rcp->current = current;

	if (rcp->updated == 0) {
		rcp->updated = 1;
		rcp->ts_flush = rcp->current;
	}

	slap = _itimediff(rcp->current, rcp->ts_flush);

	if (slap >= 10000 || slap < -10000) {
		rcp->ts_flush = rcp->current;
		slap = 0;
	}

	if (slap >= 0) {
		rcp->ts_flush += rcp->interval;
		if (_itimediff(rcp->current, rcp->ts_flush) >= 0) {
			rcp->ts_flush = rcp->current + rcp->interval;
		}
		ircp_flush(rcp);
	}
}


//---------------------------------------------------------------------
// Determine when should you invoke ircp_update:
// returns when you should invoke ircp_update in millisec, if there 
// is no ircp_input/_send calling. you can call ircp_update in that
// time, instead of call update repeatly.
// Important to reduce unnacessary ircp_update invoking. use it to 
// schedule ircp_update (eg. implementing an epoll-like mechanism, 
// or optimize ircp_update when handling massive rcp connections)
//---------------------------------------------------------------------
IUINT32 ircp_check(const ircpcb *rcp, IUINT32 current)
{
	IUINT32 ts_flush = rcp->ts_flush;
	IINT32 tm_flush = 0x7fffffff;
	IINT32 tm_packet = 0x7fffffff;
	IUINT32 minimal = 0;
	struct IQUEUEHEAD *p;

	if (rcp->updated == 0) {
		return current;
	}

	if (_itimediff(current, ts_flush) >= 10000 ||
		_itimediff(current, ts_flush) < -10000) {
		ts_flush = current;
	}

	if (_itimediff(current, ts_flush) >= 0) {
		return current;
	}

	tm_flush = _itimediff(ts_flush, current);

	for (p = rcp->snd_buf.next; p != &rcp->snd_buf; p = p->next) {
		const IRCPSEG *seg = iqueue_entry(p, const IRCPSEG, node);
		IINT32 diff = _itimediff(seg->resendts, current);
		if (diff <= 0) {
			return current;
		}
		if (diff < tm_packet) tm_packet = diff;
	}

	minimal = (IUINT32)(tm_packet < tm_flush ? tm_packet : tm_flush);
	if (minimal >= rcp->interval) minimal = rcp->interval;

	return current + minimal;
}



int ircp_setmtu(ircpcb *rcp, int mtu)
{
	char *buffer;
	if (mtu < 50 || mtu < (int)IRCP_OVERHEAD)
		return -1;
	buffer = (char*)ircp_malloc((mtu + IRCP_OVERHEAD) * 3);
	if (buffer == NULL)
		return -2;
	rcp->mtu = mtu;
	rcp->mss = rcp->mtu - IRCP_OVERHEAD;
	ircp_free(rcp->buffer);
	rcp->buffer = buffer;
	return 0;
}

int ircp_interval(ircpcb *rcp, int interval)
{
	if (interval > 5000) interval = 5000;
	else if (interval < 10) interval = 10;
	rcp->interval = interval;
	return 0;
}

int ircp_nodelay(ircpcb *rcp, int nodelay, int interval, int resend, int nc, int rd)
{
	if (nodelay >= 0) {
		rcp->nodelay = nodelay;
		if (nodelay) {
			rcp->rx_minrto = IRCP_RTO_NDL;
		}
		else {
			rcp->rx_minrto = IRCP_RTO_MIN;
		}
	}
	if (interval >= 0) {
		if (interval > 5000) interval = 5000;
		else if (interval < 10) interval = 10;
		rcp->interval = interval;
	}
	if (resend >= 0) {
		rcp->fastresend = resend;
	}
	if (nc >= 0) {
		rcp->nocwnd = nc;
	}

	if (rd > 9) rd = 9;
	if (rd < 0) rd = 0;

	rcp->max_redu = rd;

	return 0;
}


int ircp_wndsize(ircpcb *rcp, int sndwnd, int rcvwnd)
{
	if (rcp) {
		if (sndwnd > 0) {
			rcp->snd_wnd = sndwnd;
		}
		if (rcvwnd > 0) {   // must >= max fragment size
			rcp->rcv_wnd = _imax_(rcvwnd, IRCP_WND_RCV);
		}
	}
	return 0;
}

int ircp_waitsnd(const ircpcb *rcp)
{
	return rcp->nsnd_buf + rcp->nsnd_que;
}


// read conv
IUINT32 ircp_getconv(const void *ptr)
{
	IUINT32 conv;
	ircp_decode32u((const char*)ptr, &conv);
	return conv;
}


