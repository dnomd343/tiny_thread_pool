#include <malloc.h>
#include "tiny_pool.h"

pool_t* tiny_pool_create(uint32_t size) {

    pool_t *pool = (pool_t*)malloc(sizeof(pool_t));

    for (int i = 0; i < 8; ++i) {
        pool->threads[i] = 0;
    }

    pool->status = PREPARING;
    pthread_mutex_init(&pool->busy_thread_num_mutex, NULL);

    pool->task_queue_front = NULL;
    pool->task_queue_rear = NULL;
    pool->task_queue_size = 0;
    pthread_mutex_init(&pool->task_queue_busy, NULL);

//    pthread_cond_init(&pool->task_queue_empty, NULL);
    pthread_cond_init(&pool->task_queue_not_empty, NULL);

    return pool;

}

void task_queue_push(pool_t *pool, task_t *task) {

    printf("push one task\n");

    // TODO: lock task queue
    pthread_mutex_lock(&pool->task_queue_busy);

    if (pool->task_queue_rear == NULL) { // empty queue

        pool->task_queue_front = task;
        pool->task_queue_rear = task;

    } else { // queue with element

        pool->task_queue_rear->next = task;
        pool->task_queue_rear = task;

    }

    ++pool->task_queue_size;

    // TODO: unlock task queue
    pthread_mutex_unlock(&pool->task_queue_busy);

    printf("push success\n");

    if (pool->status == RUNNING) {

        // TODO: send cond signal

        printf("send signal -> queue not empty\n");

        pthread_cond_signal(&pool->task_queue_not_empty);

    }


}

void tiny_pool_submit(pool_t *pool, void (*func)(void*), void *arg) {

    task_t *new_task = (task_t*)malloc(sizeof(task_t));

    new_task->func = func;
    new_task->arg = arg;
    new_task->next = NULL;

    // TODO: new task push into task queue
    task_queue_push(pool, new_task);

}

task_t* task_queue_pop(pool_t *pool) {

    printf("try pop one task\n");

    // TODO: lock task queue
    pthread_mutex_lock(&pool->task_queue_busy);

    while (pool->task_queue_front == NULL) {

        printf("pop enter wait\n");

        // TODO: wait new task added
        pthread_cond_wait(&pool->task_queue_not_empty, &pool->task_queue_busy);

    }

    printf("pop exit wait\n");

    task_t *front = pool->task_queue_front;

    if (pool->task_queue_front == pool->task_queue_rear) { // only one element

        // queue is empty now
        pool->task_queue_front = NULL;
        pool->task_queue_rear = NULL;

    } else {

        pool->task_queue_front = front->next;

    }

    // TODO: unlock task queue
    pthread_mutex_unlock(&pool->task_queue_busy);

    printf("pop success\n");

    return front;

}


void* thread_entry(void *pool_ptr) {

    // TODO: main loop for one thread

    // TODO: check if thread pool exiting

    // TODO: pop one task --failed--> blocking wait
    //                    --success--> start running and then free task_t

    pool_t *pool = (pool_t*)pool_ptr;

    while (pool->status != EXITING) {

        printf("thread working\n");

        task_t *task = task_queue_pop(pool);

        // task working
        task->func(task->arg);

        free(task);

    }

    printf("sub thread exit\n");

    return NULL;

}


void tiny_pool_boot(pool_t *pool) {

    // TODO: create admin thread

    // TODO: create N work-threads (using N = 8 in dev)

    // TODO: avoid booting multi-times


    pthread_mutex_lock(&pool->task_queue_busy);

    for (uint32_t i = 0; i < 8; ++i) {

        printf("start thread %d\n", i);

        pthread_create(&(pool->threads[i]), NULL, thread_entry, (void*)pool);

    }

    printf("thread boot complete\n");

    pool->status = RUNNING;

    pthread_mutex_unlock(&pool->task_queue_busy);

}

