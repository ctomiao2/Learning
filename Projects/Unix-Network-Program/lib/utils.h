#ifndef __UTILS_H__
#define __UTILS_H__

#include "ikcp.h"

#define MAXLINE 1024
#define LISTENQ 100


IUINT32 iclock();

void err_quit(const char * s);

IUINT32 get_user_conv_from_kcp_str(const char *s, int size);

void print_decode_kcp_str(const char *s, int size);

#endif
