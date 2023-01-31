#include <malloc.h>
#include "tiny_pool.h"

pool_t* tiny_pool_create(uint32_t size) {

    pool_t *pool = (pool_t*)malloc(sizeof(pool_t));

    for (int i = 0; i < 8; ++i) {
        pool->threads[i] = 0;
    }

    pool->task_queue_front = NULL;
    pool->task_queue_rear = NULL;
    pool->task_queue_size = 0;

    return pool;

}

void tiny_pool_submit(pool_t *pool, void* (*func)(void*), void *arg) {

    task_t *new_task = (task_t*)malloc(sizeof(task_t));

    new_task->func = func;
    new_task->arg = arg;
    new_task->next = NULL;

    // TODO: lock task queue

    if (pool->task_queue_rear == NULL) { // queue without element

        pool->task_queue_front = new_task;
        pool->task_queue_rear = new_task;

    } else { // queue emplace back

        pool->task_queue_rear->next = new_task;
        pool->task_queue_rear = new_task;

    }

    ++pool->task_queue_size;

    // TODO: unlock task queue

}

task_t* task_queue_pop(pool_t *pool) {

    // TODO: lock task queue

    if (pool->task_queue_front == NULL) {

        return NULL; // pop failed -> empty queue

    }

    task_t *tmp = pool->task_queue_front;

    if (pool->task_queue_front == pool->task_queue_rear) {

        // queue is empty now
        pool->task_queue_front = NULL;
        pool->task_queue_rear = NULL;

    } else {

        pool->task_queue_front = tmp->next;

    }

    // TODO: unlock task queue

    return tmp;

}


void* thread_working() {

    // TODO: main loop for one thread

    // TODO: check if thread pool exiting

    // TODO: pop one task --failed--> blocking wait
    //                    --success--> start running and then free task_t

    return NULL;

}


void tiny_pool_boot(pool_t *pool) {

    // TODO: create admin thread

    // TODO: create N work-threads (using N = 8 in dev)

}

