#include "threadpool.h"
#include <stdbool.h>
#include <stdio.h>

#define THREAD_NUM 4
// test function
void test_function (void * p_argument)
{
    int * p_value = (int *)p_argument;
    printf("test_function: %d\n", *p_value);
}

int main (int argc, char * argv[])
{
    threadpool_t * p_pool = threadpool_create(THREAD_NUM);

    // submitting jobs
    for (int i = 0; i < 30; i++)
    {
        task_t task = { .p_task_function = test_function,
                        .p_argument      = &i,
                        .exit            = false };
        fprintf(stdout, "threadpool queue size: %d\n", p_pool->size);
        threadpool_enqueue(p_pool, task);
    }

    threadpool_exit(p_pool);
    threadpool_destroy(p_pool);

    fprintf(stdout, "all jobs finished\n");

    return 0;
}
