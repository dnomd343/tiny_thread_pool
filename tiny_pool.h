#ifndef TINY_POOL_H_
#define TINY_POOL_H_

#include <stdint.h>
#include <pthread.h>

typedef struct task_t {
    void (*func)(void*);
    void *arg;
    struct task_t *next;
} task_t;

// TODO: thread pool status -> preparing / running / exiting / exited
enum tiny_pool_status {
    PREPARING,
    RUNNING,
    EXITING,
};

typedef struct {

    pthread_t *threads;
    uint32_t thread_num;

    enum tiny_pool_status status;
    pthread_mutex_t status_changing;

    uint32_t busy_thread_num;
    pthread_mutex_t busy_thread_num_mutex;

    task_t *task_queue_front;
    task_t *task_queue_rear;
    uint32_t task_queue_size;
    pthread_mutex_t task_queue_busy;

//    pthread_cond_t task_queue_empty;
    pthread_cond_t task_queue_not_empty;

} pool_t;


pool_t* tiny_pool_create(uint32_t size);

void tiny_pool_submit(pool_t *pool, void (*func)(void*), void *arg);

// TODO: confirm just run once
void tiny_pool_boot(pool_t *pool);

void tiny_pool_kill(pool_t *pool);

// TODO: destroy method

// pool join -> handle to remain tasks -> return when queue empty and not thread working

// pool destroy -> only wait current working task -> ignore waiting tasks in queue -> free memory and exit

#endif
