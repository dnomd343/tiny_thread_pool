#include <malloc.h>
#include <memory.h>
#include "tiny_pool.h"

pool_t* tiny_pool_create(uint32_t size) {
    /// thread pool struct create
    pool_t *pool = (pool_t*)malloc(sizeof(pool_t));
    if (pool == NULL) {
        return NULL; // malloc pool failed -> stop create
    }

    /// threads memory initialize
    pool->thread_num = size;
    pool->threads = (pthread_t*)malloc(sizeof(pthread_t) * size);
    if (pool->threads == NULL) {
        free(pool);
        return NULL; // malloc threads failed -> stop create
    }
    memset(pool->threads, 0, sizeof(pthread_t) * size); // clean thread ids as zero

    /// variable initialization
    pool->busy_thr_num = 0;
    pool->status = PREPARING;
    pool->task_queue_size = 0;
    pool->task_queue_rear = NULL;
    pool->task_queue_front = NULL;

    /// thread mutex initialization
    if (pthread_mutex_init(&pool->status_mutex, NULL)) {
        free(pool->threads);
        free(pool);
        return NULL; // status mutex init error -> stop create
    }
    if (pthread_mutex_init(&pool->task_queue_busy, NULL)) {
        pthread_mutex_destroy(&pool->status_mutex);
        free(pool->threads);
        free(pool);
        return NULL; // queue mutex init error -> stop create
    }
    if (pthread_mutex_init(&pool->busy_thr_num_mutex, NULL)) {
        pthread_mutex_destroy(&pool->task_queue_busy);
        pthread_mutex_destroy(&pool->status_mutex);
        free(pool->threads);
        free(pool);
        return NULL; // busy thread num mutex init error -> stop create
    }

    /// thread condition variable initialization
    if (pthread_cond_init(&pool->task_queue_empty, NULL)) {
        pthread_mutex_destroy(&pool->busy_thr_num_mutex);
        pthread_mutex_destroy(&pool->task_queue_busy);
        pthread_mutex_destroy(&pool->status_mutex);
        free(pool->threads);
        free(pool);
        return NULL; // pthread cond init error -> stop create
    }
    if (pthread_cond_init(&pool->task_queue_not_empty, NULL)) {
        pthread_cond_destroy(&pool->task_queue_empty);
        pthread_mutex_destroy(&pool->busy_thr_num_mutex);
        pthread_mutex_destroy(&pool->task_queue_busy);
        pthread_mutex_destroy(&pool->status_mutex);
        free(pool->threads);
        free(pool);
        return NULL;
    }
    return pool; // tiny thread pool create success
}

void task_queue_push(pool_t *pool, task_t *task) {

    printf("push one task\n");

    pthread_mutex_lock(&pool->task_queue_busy); // lock task queue
    if (pool->task_queue_rear == NULL) { // task queue is empty
        pool->task_queue_front = task;
        pool->task_queue_rear = task; // init task queue with one element
    } else {
        pool->task_queue_rear->next = task; // task queue push back
        pool->task_queue_rear = task;
    }
    ++pool->task_queue_size;

    printf("push success -> size = %d\n", pool->task_queue_size);

    pthread_mutex_unlock(&pool->task_queue_busy); // unlock task queue
    if (pool->status > PREPARING) { // avoid send signal in PREPARING stage

        printf("signal -> queue not empty\n");

        pthread_cond_signal(&pool->task_queue_not_empty); // active one blocking thread
    }
}

task_t* task_queue_pop(pool_t *pool) { // pop one task with blocking wait

    printf("%lu -> try pop one task\n", pthread_self());

    pthread_mutex_lock(&pool->task_queue_busy); // lock task queue
    while (pool->task_queue_front == NULL) { // loop until task queue not empty

        printf("%lu -> pop start wait\n", pthread_self());

        // TODO: at EXITING process may receive active broadcast -> we should stop pop task here
        //       should we cancel thread here directly, or return NULL for sub thread loop?
        pthread_cond_wait(&pool->task_queue_not_empty, &pool->task_queue_busy); // wait new task added

        printf("%lu -> pop exit wait\n", pthread_self());

    }

    printf("%lu -> pop new task\n", pthread_self());

    bool queue_empty = false;
    task_t *front = pool->task_queue_front;
    if (pool->task_queue_front == pool->task_queue_rear) { // only one element
        pool->task_queue_front = NULL; // clear task queue
        pool->task_queue_rear = NULL;
        queue_empty = true;
    } else {
        pool->task_queue_front = front->next; // pop first task
    }
    --pool->task_queue_size;

    printf("%lu -> pop success -> size = %d\n", pthread_self(), pool->task_queue_size);

    pthread_mutex_unlock(&pool->task_queue_busy); // unlock task queue

    if (queue_empty) { // send signal after mutex unlocked
        pthread_cond_signal(&pool->task_queue_empty); // active blocking pool join thread
    }
    return front; // success pop one task
}

bool tiny_pool_submit(pool_t *pool, void (*func)(void*), void *arg) {
    task_t *new_task = (task_t*)malloc(sizeof(task_t));
    if (new_task == NULL) {
        return false; // malloc new task error -> stop submit
    }
    new_task->func = func; // load custom task
    new_task->arg = arg;
    new_task->next = NULL;

    // TODO: warning -> check dead lock here
    pthread_mutex_lock(&pool->status_mutex);
    if (pool->status > RUNNING) {
        free(new_task);
        return false; // adding task is prohibited after STOPPING
    } else {
        task_queue_push(pool, new_task); // push into task queue
    }
    pthread_mutex_unlock(&pool->status_mutex);
    return true; // task push success
}

void* thread_entry(void *pool_ptr) { // main loop for sub thread
    pool_t *pool = (pool_t*)pool_ptr;

    printf("start thread %lu\n", pthread_self());

    while (pool->status != EXITING) { // loop until enter EXITING mode

        printf("%lu -> sub thread working\n", pthread_self());

        task_t *task = task_queue_pop(pool); // pop one task -> blocking function

        pthread_mutex_lock(&pool->busy_thr_num_mutex);
        ++pool->busy_thr_num; // change busy thread number
        pthread_mutex_unlock(&pool->busy_thr_num_mutex);

        task->func(task->arg); // start running task function

        pthread_mutex_lock(&pool->busy_thr_num_mutex);
        --pool->busy_thr_num; // change busy thread number
        pthread_mutex_unlock(&pool->busy_thr_num_mutex);

        free(task); // free allocated memory
    }

    printf("%lu -> sub thread exit\n", pthread_self());

    return NULL; // sub thread exit
}

// TODO: should we return a bool value?
void tiny_pool_boot(pool_t *pool) {

    // TODO: avoid booting multi-times

    pthread_mutex_lock(&pool->status_mutex);

    if (pool->status != PREPARING) {
        pthread_mutex_unlock(&pool->status_mutex);
        return;
    }

    pthread_mutex_lock(&pool->task_queue_busy);

    for (uint32_t i = 0; i < pool->thread_num; ++i) {
        pthread_create(&(pool->threads[i]), NULL, thread_entry, (void*)pool);
    }

    printf("thread boot complete\n");

    pool->status = RUNNING;

    pthread_mutex_unlock(&pool->task_queue_busy);
    pthread_mutex_unlock(&pool->status_mutex);

}

bool tiny_pool_join(pool_t *pool) {

    // TODO: set status -> JOINING -> avoid submit

    // TODO: wait --until--> queue empty

    // TODO: set status -> EXITING -> some thread may exit

    // TODO: signal broadcast -> wait all thread exit


    printf("start pool join\n");

    pthread_mutex_lock(&pool->status_mutex);

    if (pool->status != RUNNING) {

        pthread_mutex_unlock(&pool->status_mutex);

        return false;

    }
    pool->status = STOPPING;


    // TODO: join process

    return true;
}

//void tiny_pool_wait(pool_t *pool) {

    // TODO: wait all tasks exit

    // TODO: check `busy_thread_num` == 0 and queue empty

//}
