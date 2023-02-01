#include <malloc.h>
#include <memory.h>
#include "tiny_pool.h"

pool_t* tiny_pool_create(uint32_t size) {

    // TODO: return NULL when error

    printf("create thread pool size -> %d\n", size);

    pool_t *pool = (pool_t*)malloc(sizeof(pool_t));

    if (pool == NULL) {
        return NULL;
    }

    pool->thread_num = size;
    pool->threads = (pthread_t*)malloc(sizeof(pthread_t) * size);

    if (pool->threads == NULL) {

        free(pool);

        return NULL;
    }

    memset(pool->threads, 0, sizeof(pthread_t) * size);

    pool->status = PREPARING;
    pthread_mutex_init(&pool->status_changing, NULL);

    pool->busy_thread_num = 0;
    pthread_mutex_init(&pool->busy_thread_num_mutex, NULL);

    pool->task_queue_front = NULL;
    pool->task_queue_rear = NULL;
    pool->task_queue_size = 0;
    pthread_mutex_init(&pool->task_queue_busy, NULL);

    pthread_cond_init(&pool->task_queue_empty, NULL);
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

    printf("push success -> size = %d\n", pool->task_queue_size);

    // TODO: unlock task queue
    pthread_mutex_unlock(&pool->task_queue_busy);


    if (pool->status == RUNNING) {

        // TODO: send cond signal

//        printf("send signal -> queue not empty\n");
//        pthread_cond_signal(&pool->task_queue_not_empty);

        printf("send broadcast -> queue not empty\n");
        pthread_cond_broadcast(&pool->task_queue_not_empty);

    }

}

void tiny_pool_submit(pool_t *pool, void (*func)(void*), void *arg) {

    // check status -> failed
    if (pool->status == EXITING) {
        return;
        // TODO: return false here
    }

    // TODO: malloc error -> return bool false
    task_t *new_task = (task_t*)malloc(sizeof(task_t));

    new_task->func = func;
    new_task->arg = arg;
    new_task->next = NULL;

    // TODO: new task push into task queue
    task_queue_push(pool, new_task);

    // TODO: queue push may failed -> return false

    // TODO: return bool true

}

task_t* task_queue_pop(pool_t *pool) {

    printf("try pop one task\n");

    // TODO: lock task queue
    pthread_mutex_lock(&pool->task_queue_busy);

    while (pool->task_queue_front == NULL) {

        printf("pop enter wait\n");

        // TODO: wait new task added
        pthread_cond_wait(&pool->task_queue_not_empty, &pool->task_queue_busy);

        printf("pop exit wait\n");

    }

    printf("pop exit loop\n");

    task_t *front = pool->task_queue_front;

    if (pool->task_queue_front == pool->task_queue_rear) { // only one element

        // queue is empty now
        pool->task_queue_front = NULL;
        pool->task_queue_rear = NULL;

        /// will it cause dead lock?
        pthread_cond_signal(&pool->task_queue_empty);

    } else {

        pool->task_queue_front = front->next;

    }

    --pool->task_queue_size;

    printf("pop success -> size = %d\n", pool->task_queue_size);

    // TODO: unlock task queue
    pthread_mutex_unlock(&pool->task_queue_busy);


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

    for (uint32_t i = 0; i < pool->thread_num; ++i) {

        printf("start thread %d\n", i);

        pthread_create(&(pool->threads[i]), NULL, thread_entry, (void*)pool);

    }

    printf("thread boot complete\n");

    pthread_mutex_lock(&pool->status_changing);
    pool->status = RUNNING;
    pthread_mutex_unlock(&pool->status_changing);

    pthread_mutex_unlock(&pool->task_queue_busy);

}

//void tiny_pool_kill(pool_t *pool) {
//
//    printf("pool enter EXITING status\n");
//
//    pthread_mutex_lock(&pool->status_changing);
//
//    pool->status = EXITING;
//
//    pthread_mutex_unlock(&pool->status_changing);
//
//}

void tiny_pool_wait(pool_t *pool) {

    // TODO: wait all tasks exit

    // TODO: check `busy_thread_num` == 0 and queue empty

}

void tiny_pool_join(pool_t *pool) {

    // TODO: set status -> JOINING -> avoid submit

    // TODO: wait --until--> queue empty

    // TODO: set status -> EXITING -> some thread may exit

    // TODO: signal broadcast -> wait all thread exit

}
