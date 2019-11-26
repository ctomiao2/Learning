#include <time.h>
#include "utils.h"

void err_quit(const char *s) {
    printf("err_quit, msg=%s\n", s);
    exit(1);
}

IUINT32 iclock()
{
    long s, u;
    IINT64 value;
    struct timeval time;
    gettimeofday(&time, NULL);
    s = time.tv_sec;
    u = time.tv_usec;
    value = ((IINT64)s) * 1000 + (u / 1000);
    return (IUINT32)(value & 0xfffffffful);
}

/* decode 8 bits unsigned int */
static inline const char *ikcp_decode8u(const char *p, unsigned char *c)
{
	*c = *(unsigned char*)p++;
	return p;
}

static inline const char *ikcp_decode16u(const char *p, unsigned short *w)
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

/* decode 32 bits unsigned int (lsb) */
static inline const char *ikcp_decode32u(const char *p, IUINT32 *l)
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


IUINT32 get_user_conv_from_kcp_str(const char *s, int size) {
    if (size < 24)
        return 0;
    IUINT32 conv;
    ikcp_decode32u(s, &conv);
    return conv;
}

void print_decode_kcp_str(const char *s, int size) {
    
	IUINT32 ts, sn, len, una, conv;
	IUINT16 wnd;
	IUINT8 cmd, frg;

	if (size < 24)
        return;
    
    char *data = s;
	data = ikcp_decode32u(data, &conv);
	data = ikcp_decode8u(data, &cmd);
	data = ikcp_decode8u(data, &frg);
	data = ikcp_decode16u(data, &wnd);
	data = ikcp_decode32u(data, &ts);
	data = ikcp_decode32u(data, &sn);
	data = ikcp_decode32u(data, &una);
	data = ikcp_decode32u(data, &len);
    
    char *cmd_str = "INVALID";
    if (cmd == 81)
        cmd_str = "CMD_PUSH";
    else if (cmd == 82)
        cmd_str = "CMD_ACK";
    else if (cmd == 83)
        cmd_str = "CMD_ASK_WINDOW";
    else if (cmd == 84)
        cmd_str = "CMD_TELL_WINDOW";

    printf("conv: %d, cmd: %s, frg: %d, wnd: %d, ts: %lld, sn: %d, una: %d, len: %d, data: %s\n", 
        conv, cmd_str, frg, wnd, ts, sn, una, len, data);
}
