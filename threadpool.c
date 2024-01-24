#include "threadpool.h"

/**
 * @brief Executes a task
 *
 * @param p_task Task to execute
 */
void threadpool_execute (task_t * p_task)
{
    if (p_task->exit)
    {
        goto EXIT;
    }

    if (p_task->p_task_function != NULL)
    {
        p_task->p_task_function(p_task->p_argument);
    }

EXIT:
    return;
}

/**
 * @brief Enqueues a task to the threadpool
 *
 * @param p_pool Threadpool to enqueue task to
 * @param task Task to enqueue
 */
void threadpool_enqueue (threadpool_t * p_pool, task_t task)
{
    pthread_mutex_lock(&(p_pool->mutex_queue));

    while ((p_pool->rear + 1) % QUEUE_SIZE == p_pool->front)
    {
        // Queue is full, wait here until queue is not full
        pthread_cond_wait(&p_pool->cond_queue_not_full, &p_pool->mutex_queue);
    }

    p_pool->task_queue[p_pool->rear] = task;
    p_pool->rear                     = (p_pool->rear + 1) % QUEUE_SIZE;
    p_pool->size++;

    pthread_mutex_unlock(&p_pool->mutex_queue);
    pthread_cond_signal(&p_pool->cond_queue);

    return;
}

/**
 * @brief Dequeues a task from the threadpool
 *
 * @param p_pool Threadpool to dequeue task from
 * @return task_t* Dequeue'd task
 */
task_t * threadpool_dequeue (threadpool_t * p_pool)
{
    pthread_mutex_lock(&p_pool->mutex_queue);

    while (p_pool->size == 0)
    {
        pthread_cond_wait(&p_pool->cond_queue, &p_pool->mutex_queue);
    }

    task_t * p_task = &p_pool->task_queue[p_pool->front];
    p_pool->front   = (p_pool->front + 1) % QUEUE_SIZE;
    p_pool->size--;

    pthread_mutex_unlock(&p_pool->mutex_queue);
    pthread_cond_signal(&p_pool->cond_queue_not_full);

    return p_task;
}

/**
 * @brief Starts a threadpool
 *
 * @param p_pool_void Threadpool to start
 * @return void* NULL
 */
void * threadpool_start (void * p_pool_void)
{
    threadpool_t * p_pool = (threadpool_t *)p_pool_void;

    if (NULL == p_pool)
    {
        perror("Failed to get threadpool");
        goto EXIT;
    }

    for (;;)
    {
        task_t * p_task = threadpool_dequeue(p_pool);
        threadpool_execute(p_task);

        if (p_task->exit)
        {
            // Exit thread when encountering the exit task
            goto EXIT;
        }
    }

EXIT:
    return NULL;
}

/**
 * @brief Signals threads to exit
 *
 * @param p_pool Threadpool to signal threads to exit
 */
void threadpool_exit (threadpool_t * p_pool)
{
    for (int thread_idx = 0; thread_idx < p_pool->num_threads; thread_idx++)
    {
        task_t exit_task = { NULL, NULL, .exit = true };
        threadpool_enqueue(p_pool, exit_task);
    }

    return;
}

/**
 * @brief Destroys a threadpool
 *
 * @param p_pool Threadpool to destroy
 */
void threadpool_destroy (threadpool_t * p_pool)
{
    for (int thread_idx = 0; thread_idx < p_pool->num_threads; thread_idx++)
    {
        if (pthread_join(p_pool->p_threadpool[thread_idx], NULL) != 0)
        {
            perror("Failed to join the thread");
        }
    }

    pthread_mutex_destroy(&p_pool->mutex_queue);
    pthread_cond_destroy(&p_pool->cond_queue);
    free(p_pool->p_threadpool);
    free(p_pool->task_queue);
    free(p_pool);
    p_pool = NULL;

    return;
}

/**
 * @brief Creates a threadpool
 *
 * @param num_threads Number of threads to create
 * @return threadpool_t* Threadpool on success, NULL otherwise
 */
threadpool_t * threadpool_create (int num_threads)
{
    threadpool_t * p_pool = (threadpool_t *)malloc(sizeof(threadpool_t));

    if (NULL == p_pool)
    {
        perror("Failed to allocate memory for threadpool");
        goto EXIT;
    }

    p_pool->task_queue = (task_t *)malloc(sizeof(task_t) * QUEUE_SIZE);

    if (NULL == p_pool->task_queue)
    {
        perror("Failed to allocate memory for task queue");
        goto EXIT;
    }

    p_pool->p_threadpool = (pthread_t *)malloc(sizeof(pthread_t) * num_threads);

    if (NULL == p_pool->p_threadpool)
    {
        perror("Failed to allocate memory for threadpool");
        goto EXIT;
    }

    p_pool->size        = 0;
    p_pool->front       = 0;
    p_pool->rear        = 0;
    p_pool->num_threads = num_threads;
    pthread_mutex_init(&p_pool->mutex_queue, NULL);
    pthread_cond_init(&p_pool->cond_queue, NULL);
    pthread_cond_init(&p_pool->cond_queue_not_full, NULL);

    for (int thread_idx = 0; thread_idx < p_pool->num_threads; thread_idx++)
    {
        if (pthread_create(&p_pool->p_threadpool[thread_idx],
                           NULL,
                           threadpool_start,
                           p_pool)
            != 0)
        {
            perror("Failed to create the thread");
            threadpool_destroy(p_pool);
            p_pool = NULL;
            goto EXIT;
        }
    }

EXIT:
    return p_pool;
}
