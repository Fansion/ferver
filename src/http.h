#ifndef __HTTP_H__
#define __HTTP_H__

#include <strings.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "rio.h"
#include "list.h"

#include "dbg.h"
#include "util.h"
#include "http_request.h"
#include "http_parse.h"

#define MAXLINE 8192
#define SHORTLINE 512
#define ROOT "/home/frank/ustc_home"

// Fast byte comparison. Inspired by Igor Sysoev's Nginx.
// big endian
#define fv_str3_cmp(m,c0,c1,c2,c3)     *(uint32_t *) m == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)
#define fv_str30cmp(m, c0, c1, c2, c3) *(uint32_t *) m == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)
#define fv_str4cmp(m, c0, c1, c2, c3)  *(uint32_t *) m == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)

typedef struct mime_type_s
{
    const char *type;
    const char *value;
} mime_type_t;

void do_request(void *infd);

#endif
