#include "../src/threadpool.h"

static void sum_n(void *arg)
{
    int n = (int)arg, i;
    unsigned long sum = 0;
    for (i = 0; i < n; ++i)
    {
        sum += i;
    }
    log_info("thread %08x complete sum between [0,%d): %lu", (unsigned int)pthread_self(), n, sum);
}

int main()
{
    int rc;
    fv_threadpool_t *tp = threadpool_init(THREAD_NUM);

    int i;
    for (i = 0; i < 100; ++i)
    {
        log_info("ready to add number between [0,%d)", i);
        rc = threadpool_add(tp, sum_n, (void *)i);
        check(rc == 0, "rc should be 0");
    }

    if (threadpool_destroy(tp) < 0) {
        log_err("destroy threadpool failed");
    }
    // 主线程需要等待一段时间退出
    // 否则当顶层主线程退出时，子线程会隐式退出
    sleep(10);
    return 0;
}
