#ifndef TINY_POOL_H_
#define TINY_POOL_H_

#include <stdint.h>
#include <pthread.h>

typedef struct task_t {
    void* (*func)(void*);
    void *arg;
    struct task_t *next;
} task_t;


typedef struct {

    pthread_t threads[8];

    task_t *task_queue_front;
    task_t *task_queue_rear;
    uint32_t task_queue_size;
    pthread_mutex_t task_queue_busy;

} pool_t;


pool_t* tiny_pool_create(uint32_t size);

void tiny_pool_submit(pool_t *pool, void* (*func)(void*), void *arg);

// TODO: confirm just run once
void tiny_pool_boot(pool_t *pool);

// TODO: thread pool status -> preparing / running / exiting / exited

// TODO: destroy method

#endif
