#ifndef __UTILS_H__
#define __UTILS_H__

#include "ikcp.h"

#define MAXLINE 1024
#define LISTENQ 100


inline IUINT32 iclock()
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

void err_quit(const char * s);

IUINT32 get_user_conv_from_kcp_str(const char *s, int size);

void print_decode_kcp_str(const char *s, int size);

#endif
