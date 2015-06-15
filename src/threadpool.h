/*
 * Copyright (c) 2013, Mathias Brossard <mathias@brossard.org>.
 * All rights reserved.
 * Modified based on https://github.com/mbrossard/threadpool
 */
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

/**
 *  @struct fv_task_t
 *  @brief the work struct
 *
 *  @var function Pointer to the function that will perform the task.
 *  @var argument Argument to be passed to the function.
 */
typedef struct fv_task_s {
    void (*func)(void*);
    void *arg;
    struct fv_task_s *next;
} fv_task_t;

/**
 *  @struct threadpool
 *  @brief The threadpool struct
 *  @var lock         mutex variable.
 *  @var cond         Condition variable to notify worker threads.
 *  @var threads      Array containing worker threads ID.
 *  @var thread_count Number of threads
 *  @var head        Array containing the task queue.
 *  @var queue_size   Size of the task queue.
 *  @var shutdown     Flag indicating if the pool is shutting down
 *  @var started      Number of started threads
 */
typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t cond;
    pthread_t *threads;
    fv_task_t *head;
    int thread_count;
    int queue_size;
    int shutdown;
    int started;
} fv_threadpool_t;

typedef enum {
    fv_tp_invalid = -1,
    fv_tp_lock_fail = -2,
    fv_tp_already_shutdown = -3,
    fv_tp_cond_broadcast = -4,
    fv_tp_thread_fail = -5
} fv_threadpool_error_t;

/**
 * @function threadpool_init
 * @brief Creates a fv_threadpool_t object
 * @param thread_num    Number of worker threads
 * @return a newly created thread pool or NULL
 */
fv_threadpool_t *threadpool_init(int thread_num);

/**
 * @function threadpool_add
 * @brief add a new task in the queue of a thread pool
 * @param pool  The thread pool
 * @param func  Pointer to the func that perform the task
 * @param arg   Argument to be passed to the func
 * @return 0 if all go well, negative values in case of error(see fv_threadpool_error_t)
 */
int threadpool_add(fv_threadpool_t *pool, void (*func)(void *), void *arg);

/**
 * @function threadpool_destroy
 * @brief stops and destroy a thread pool
 * @param pool      The thread pool to destroy
 * @param graceful  Flags for shutdown
 *
 * Known values for flags are 0 (default) and graceful_shutdown in
 * which case the thread pool doesn't accept any new tasks but
 * processes all pending tasks before shutdown.
 */
int threadpool_destroy(fv_threadpool_t *pool, int graceful);

#ifdef __cplusplus
}
#endif

#endif
