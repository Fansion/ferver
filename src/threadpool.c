/*
 * Copyright (c) 2013, Mathias Brossard <mathias@brossard.org>.
 * All rights reserved.
 * Modified based on https://github.com/mbrossard/threadpool
 */
#include "threadpool.h"

typedef enum {
    immediate_shutdown = 1,
    graceful_shutdown = 2
} fv_threadpool_sd_t;

static void *threadpool_worker(void *arg);
int threadpool_free(fv_threadpool_t *pool);

fv_threadpool_t *threadpool_init(int thread_num)
{
    /* TODO: Check for negative or otherwise very big input parameters */
    if (thread_num <= 0) {
        log_err("the arg of threadpool_init must > 0");
        return NULL;
    }
    /* Initialize */
    fv_threadpool_t *pool;
    if ((pool = (fv_threadpool_t*)malloc(sizeof(fv_threadpool_t))) == NULL) {
        goto err;
    }
    pool->thread_count = 0;
    pool->queue_size = 0;
    pool->shutdown = 0;
    pool->started = 0;
    /* Allocate thread and task queue */
    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_num);
    /*此处保留一个空头部，便于插入删除操作*/
    pool->head = (fv_task_t *)malloc(sizeof(fv_task_t));
    pool->head->func = pool->head->arg = pool->head->next = NULL;
    /* Initialize mutex and conditional variable first */
    if (pthread_mutex_init(&(pool->lock), NULL) != 0 ||
            pthread_cond_init(&(pool->cond), NULL) != 0 ||
            pool->threads == NULL ||
            pool->head == NULL)
    {
        goto err;
    }
    /* Start worker threads */
    int i;
    for (i = 0; i < thread_num; ++i)
    {
        if (pthread_create(&(pool->threads[i]), NULL, threadpool_worker, (void *)pool) != 0)
        {
            threadpool_destroy(pool, 0);
            return NULL;
        }
        log_info("thread: %08x started", (unsigned int)pool->threads[i]);
        pool->thread_count++;
        pool->started++;
    }
    return pool;
err:
    if (pool)
    {
        threadpool_free(pool);
    }
    return NULL;
}

static void *threadpool_worker(void *arg)
{
    if (arg == NULL)
    {
        log_err("arg should be type fv_threadpool_t*");
        exit(0);
    }
    fv_threadpool_t *pool = (fv_threadpool_t*)arg;
    fv_task_t * task;
    while (1) {
        /* lock must be taken to wait on condition variable */
        pthread_mutex_lock(&(pool->lock));
        /*  Wait on condition variable, check for spurious wakeups.
         *  When returning from pthread_cond_wait(), thread owns the lock
         */
        while (pool->queue_size == 0 && !pool->shutdown) {
            log_info("%08x goto sleep", (int)pthread_self());
            pthread_cond_wait(&(pool->cond), &(pool->lock));
        }
        log_info("%08x wake up", (int)pthread_self());
        if (pool->shutdown == immediate_shutdown) {
            pthread_mutex_unlock(&(pool->lock));
            break;
        } else if (pool->shutdown == graceful_shutdown &&
                   pool->queue_size == 0) {
            /*
             *  graceful_shutdown means the thread pool doesn't accept any new tasks
             *  but processes all pending tasks before shutdown
             */
            pthread_mutex_unlock(&(pool->lock));
            break;
        }
        /* grab the task */
        task = pool->head->next;
        if (task == NULL)
        {
            continue;
        }
        pool->head->next = task->next;
        pool->queue_size--;
        pthread_mutex_unlock(&(pool->lock));
        (*(task->func))(task->arg);
        log_info("%08x complete its job", (int)pthread_self());
        free(task);
    }
    pool->started--;
    pthread_mutex_unlock(&(pool->lock));
    pthread_exit(NULL);
}

int threadpool_add(fv_threadpool_t *pool, void (*func)(void *), void *arg)
{
    int rc = 0, err = 0;
    if (pool == NULL || func == NULL)
    {
        log_err("pool == NULL || func == NULL");
        return fv_tp_invalid;
    }
    if (pthread_mutex_lock(&(pool->lock)) != 0)
    {
        log_err("pthread_mutex_lock");
        return fv_tp_lock_fail;
    }

    if (pool->shutdown)
    {
        err = fv_tp_already_shutdown;
        goto out;
    }

    fv_task_t *task = (fv_task_t*)malloc(sizeof(fv_task_t));
    // TODO: use a memory pool
    task->func = func;
    task->arg = arg;
    // 新节点作为空头部的下一个节点, 新任务从任务结构体的头部插入, FILO
    task->next = pool->head->next;
    pool->head->next = task;
    pool->queue_size++;

    rc = pthread_cond_signal(&(pool->cond));
    check(rc == 0, "pthread_cond_signal");

out:
    if (pthread_mutex_unlock(&(pool->lock)) != 0)
    {
        log_err("pthread_mutex_unlock");
        err = fv_tp_lock_fail;
    }
    return err;
}

int threadpool_destroy(fv_threadpool_t *pool, int graceful)
{
    int err = 0;
    if (pool == NULL)
    {
        log_err("pool == NULL");
        return fv_tp_invalid;
    }
    /* to set the showdown flag of pool and wake up all thread */
    if (pthread_mutex_lock(&(pool->lock)) != 0)
    {
        return fv_tp_lock_fail;
    }
    do {
        if (pool->shutdown)
        {
            err = fv_tp_already_shutdown;
            break;
        }
        pool->shutdown = graceful ? graceful_shutdown : immediate_shutdown;
        /* wake up all worker threads */
        if (pthread_cond_broadcast(&(pool->cond)) != 0)
        {
            err = fv_tp_cond_broadcast;
            break;
        }
        if (pthread_mutex_unlock(&(pool->lock)) != 0)
        {
            err = fv_tp_lock_fail;
            break;
        }
        int i;
        /* join all worker threads */
        for (i = 0; i < pool->thread_count; ++i)
        {
            if (pthread_join(pool->threads[i], NULL) != 0)
            {
                err = fv_tp_thread_fail;
            }
            log_info("thread %08x exit", (unsigned int)pool->threads[i]);
        }
    } while (0);
    /* Only if everything went well do we deallocate the pool */
    if (!err)
    {
        threadpool_free(pool);
    }
    return err;
}

int threadpool_free(fv_threadpool_t *pool)
{
    if (pool == NULL || pool->started > 0)
    {
        return fv_tp_invalid;
    }
    /* pool->head is a dummy head */
    fv_task_t *cur = pool->head->next, *pre;
    while (cur) {
        pre = cur;
        cur = cur->next;
        free(pre);
    }
    if (pool->threads) {
        free(pool->threads);
        free(pool->head);

        /* we allocate pool->threads after initializing the mutex and condition variable,
          we are sure they are initialized, lock the mutex just in case */
        pthread_mutex_lock(&(pool->lock));
        pthread_mutex_destroy(&(pool->lock));
        pthread_cond_destroy(&(pool->cond));
    }
    free(pool);
    return 0;
}
