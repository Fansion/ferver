#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "dbg.h"

#define THREAD_NUM 8

typedef struct fv_task_s {
    void (*func)(void*);
    void *arg;
    struct fv_task_s *next;
} fv_task_t;

typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t cond;
    pthread_t *threads;
    fv_task_t *head;
    int thread_count;
    int queue_size;
} fv_threadpool_t;;

fv_threadpool_t *threadpool_init(int thread_num);
int threadpool_add(fv_threadpool_t *pool, void (*func)(void *), void *arg);
int threadpool_destroy(fv_threadpool_t *pool);
int threadpool_free(fv_threadpool_t *pool);

#ifdef __cplusplus
}
#endif

#endif
