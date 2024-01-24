#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifndef THREADPOOL_H
#define THREADPOOL_H

#define QUEUE_SIZE 256

typedef struct task_t
{
    void (*p_task_function)(void *);
    void * p_argument;
    bool   exit;
    // Exit bool is used to tell a thread to exit.
} task_t;

typedef struct threadpool_t
{
    pthread_mutex_t mutex_queue;
    pthread_cond_t  cond_queue;
    pthread_cond_t  cond_queue_not_full;
    pthread_t *     p_threadpool;
    task_t *        task_queue;
    int             size;
    int             front;
    int             rear;
    int             num_threads;
} threadpool_t;

threadpool_t * threadpool_create (int num_threads);
void           threadpool_destroy (threadpool_t * p_threadpool);
void           threadpool_enqueue (threadpool_t * p_threadpool, task_t task);
void           threadpool_exit (threadpool_t * p_threadpool);
void           threadpool_destroy (threadpool_t * p_threadpool);

#endif