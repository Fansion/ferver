#include "threadpool.h"

static void *threadpool_worker(void *arg);

fv_threadpool_t *threadpool_init(int thread_num)
{
    if (thread_num <= 0) {
        log_err("the arg of threadpool_init must > 0");
        return NULL;
    }
    fv_threadpool_t *pool;
    if ((pool = (fv_threadpool_t*)malloc(sizeof(fv_threadpool_t))) == NULL) {
        goto err;
    }
    pool->thread_count = 0;
    pool->queue_size = 0;
    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_num);
    /*此处保留一个空头部，便于插入删除操作*/
    pool->head = (fv_task_t *)malloc(sizeof(fv_task_t));
    pool->head->func = pool->head->arg = pool->head->next = NULL;

    if (pool->threads == NULL || pool->head == NULL)
    {
        goto err;
    }
    if (pthread_mutex_init(&(pool->lock), NULL) != 0)
    {
        goto err;
    }
    if (pthread_cond_init(&(pool->cond), NULL) != 0)
    {
        pthread_mutex_lock(&(pool->lock));
        pthread_mutex_destroy(&(pool->lock));
        goto err;
    }

    int i, rc;
    for (i = 0; i < thread_num; ++i)
    {
        rc = pthread_create(&(pool->threads[i]), NULL, threadpool_worker, (void *)pool);
        if (rc != 0)
        {
            threadpool_destroy(pool);
            return NULL;
        }
        pool->thread_count++;
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
        pthread_mutex_lock(&(pool->lock));
        /*  Wait on condition variable, check for spurious wakeups. */
        while (pool->queue_size == 0) {
            log_info("%08x goto sleep", (int)pthread_self());
            pthread_cond_wait(&(pool->cond), &(pool->lock));
        }
        log_info("%08x wake up", (int)pthread_self());
        // 处理空头部的下一个节点
        task = pool->head->next;
        if (task == NULL)
        {
            log_info("empty task list now");
            continue;
        }
        pool->head->next = task->next;
        pool->queue_size--;
        pthread_mutex_unlock(&(pool->lock));
        (*(task->func))(task->arg);
        log_info("%08x complete its job", (int)pthread_self());
        free(task);
    }
    pthread_mutex_unlock(&(pool->lock));
    pthread_exit(NULL);
}


int threadpool_add(fv_threadpool_t *pool, void (*func)(void *), void *arg)
{
    int rc;
    if (pool == NULL || func == NULL)
    {
        log_err("pool == NULL || func == NULL");
        return -1;
    }
    if (pthread_mutex_lock(&(pool->lock)) != 0)
    {
        return -1;
    }

    fv_task_t *task = (fv_task_t*)malloc(sizeof(fv_task_t));
    if (task == NULL)
    {
        log_err("add task fail");
        return -1;
    }
    task->func = func;
    task->arg = arg;
    // 新节点作为空头部的下一个节点
    task->next = pool->head->next;
    pool->head->next = task;
    pool->queue_size++;

    rc = pthread_cond_signal(&(pool->cond));
    check(rc == 0, "pthread_cond_signal");

    if (pthread_mutex_unlock(&(pool->lock)) != 0)
    {
        return -1;
    }
    return 0;
}

int threadpool_destroy(fv_threadpool_t *pool)
{
    if (pool == NULL)
    {
        log_err("pool == NULL");
        return -1;
    }
    // to set the showdown flag of pool and wake up all thread
    return 0;
}

int threadpool_free(fv_threadpool_t *pool)
{
    if (pool == NULL)
    {
        log_err("pool == NULL");
        return -1;
    }
    if (pool->threads)
        free(pool->threads);
    /* pool->head is a dummy head */
    fv_task_t *cur = pool->head->next, *pre;
    while (cur) {
        pre = cur;
        cur = cur->next;
        free(pre);
    }
    free(pool);
    return 0;
}
