#ifndef __DBG_H__
#define __DBG_H__

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#ifdef NDEBUG
#define debug(M, ...)
#else
#define debug(M, ...) fprintf(stderr, "DEBUG %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif

#define clean_errno() (errno == 0 ? "None" : strerror(errno))

#define log_err(M, ...) fprintf(stderr, "[ERROR] (%s:%d errno: %s) " M "\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__);

#define log_info(M, ...) fprintf(stderr, "[INFO] (%s:%d) " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define log_warn(M, ...) fprintf(stderr, "[warn] (%s:%d errno: %s) " M "\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__);

#define check(A, M, ...) if(!(A)) { log_err(M,  ##__VA_ARGS__); exit(1); }
#define check_bug(A, M, ...) if(!(A)) { debug(M,  ##__VA_ARGS__); exit(1); }

#endif
