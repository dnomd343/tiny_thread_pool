#include <malloc.h>
#include <memory.h>
#include "tiny_pool.h"

pool_t* tiny_pool_create(uint32_t size) {
    /// thread pool struct create
    pool_t *pool = (pool_t*)malloc(sizeof(pool_t));
    if (pool == NULL) {
        return NULL; // malloc pool failed -> revoke create
    }

    /// threads memory initialize
    pool->thread_num = size;
    pool->threads = (pthread_t*)malloc(sizeof(pthread_t) * size);
    if (pool->threads == NULL) {
        free(pool);
        return NULL; // malloc threads failed -> revoke create
    }
    memset(pool->threads, 0, sizeof(pthread_t) * size); // clean thread ids as zero

    /// variable initialization
    pool->busy_thr_num = 0;
    pool->status = PREPARING;
    pool->task_queue_size = 0;
    pool->task_queue_rear = NULL;
    pool->task_queue_front = NULL;

    /// mutex and conditions initialization
    if (pthread_mutex_init(&pool->mutex, NULL)) {
        free(pool->threads);
        free(pool);
        return NULL; // global mutex init error -> revoke create
    }
    if (pthread_cond_init(&pool->task_queue_empty, NULL)) {
        pthread_mutex_destroy(&pool->mutex);
        free(pool->threads);
        free(pool);
        return NULL; // task queue cond init error -> revoke create
    }
    if (pthread_cond_init(&pool->task_queue_not_empty, NULL)) {
        pthread_cond_destroy(&pool->task_queue_empty);
        pthread_mutex_destroy(&pool->mutex);
        free(pool->threads);
        free(pool);
        return NULL; // task queue cond init error -> revoke create
    }
    if (pthread_cond_init(&pool->without_busy_thread, NULL)) {
        pthread_cond_destroy(&pool->task_queue_not_empty);
        pthread_cond_destroy(&pool->task_queue_empty);
        pthread_mutex_destroy(&pool->mutex);
        free(pool->threads);
        free(pool);
        return NULL; // busy thread num cond init error -> revoke create
    }
    return pool; // tiny thread pool create success
}

void task_queue_push(pool_t *pool, task_t *task) {
    printf("push new task %d\n", *(int*)task->arg);
    if (pool->task_queue_rear == NULL) { // task queue is empty
        pool->task_queue_front = task;
        pool->task_queue_rear = task; // init task queue with one element
    } else {
        pool->task_queue_rear->next = task; // task queue push back
        pool->task_queue_rear = task;
    }
    ++pool->task_queue_size;
    printf("push success -> size = %d\n", pool->task_queue_size);
}

task_t* task_queue_pop(pool_t *pool) { // pop one task with blocking wait
    printf("%lu -> start pop task\n", pthread_self());

    /// wait until task queue not empty
    pthread_mutex_lock(&pool->mutex); // lock pool struct
    while (pool->task_queue_front == NULL) { // loop until task queue not empty
        printf("%lu -> pop wait\n", pthread_self());
        pthread_cond_wait(&pool->task_queue_not_empty, &pool->mutex); // wait new task added
        printf("%lu -> pop exit wait\n", pthread_self());
        if (pool->status == EXITING) {
            printf("%lu -> sub thread exit from idle\n", pthread_self());
            pthread_exit(NULL); // sub thread exit at EXITING stage
        }
    }
    printf("%lu -> pop new task %d\n", pthread_self(), *(int*)pool->task_queue_front->arg);

    /// pop first task from queue
    bool empty_flag = false;
    task_t *front = pool->task_queue_front;
    pool->task_queue_front = front->next; // pop first task
    if (front->next == NULL) { // only one element
        pool->task_queue_rear = NULL; // clear task queue
        empty_flag = true;
    }
    --pool->task_queue_size;
    ++pool->busy_thr_num; // task must pop by one ready thread
    printf("%lu -> pop success -> size = %d\n", pthread_self(), pool->task_queue_size);
    pthread_mutex_unlock(&pool->mutex); // unlock task queue

    /// send signal to active blocking thread
    if (empty_flag) { // send signal after mutex unlocked
        printf("signal -> task queue empty\n");
        pthread_cond_signal(&pool->task_queue_empty); // active pool join thread
    }
    return front; // success pop one task
}

bool tiny_pool_submit(pool_t *pool, void (*func)(void*), void *arg) {
    /// pre-check to avoid invalid mutex waiting
    if (pool->status > RUNNING) {
        return false; // allow to add task at PREPARING and RUNNING stage
    }

    /// initialize task structure
    task_t *new_task = (task_t*)malloc(sizeof(task_t));
    if (new_task == NULL) {
        return false; // malloc new task error -> failed submit
    }
    new_task->entry = func; // load custom task
    new_task->arg = arg;
    new_task->next = NULL;

    /// handle task submit
    pthread_mutex_lock(&pool->mutex);
    if (pool->status > RUNNING) { // pool stage recheck after mutex lock
        pthread_mutex_unlock(&pool->mutex);
        free(new_task);
        return false; // adding task is prohibited after RUNNING
    }
    task_queue_push(pool, new_task); // push into task queue
    bool signal_flag = false;
    if (pool->status > PREPARING) { // only send active signal at RUNNING stage
        signal_flag = true;
    }
    pthread_mutex_unlock(&pool->mutex); // send signal after mutex unlock -> reduce thread churn

    /// send signal to active blocking thread
    if (signal_flag) {
        printf("signal -> queue not empty\n");
        pthread_cond_signal(&pool->task_queue_not_empty); // active one blocking thread
    }
    return true; // task push success
}

void* thread_entry(void *pool_ptr) { // main loop for sub thread
    pool_t *pool = (pool_t*)pool_ptr;
    printf("%lu -> sub thread begin\n", pthread_self());
    while (pool->status != EXITING) { // loop until enter EXITING stage
        printf("%lu -> sub thread working\n", pthread_self());

        /// pop a task and execute it
        task_t *task = task_queue_pop(pool); // pop one task -> blocking function
        task->entry(task->arg); // start running task function
        free(task); // free allocated memory

        /// mark thread as idle
        pthread_mutex_lock(&pool->mutex);
        --pool->busy_thr_num; // change busy thread number
        bool busy_flag = (pool->busy_thr_num == 0);
        pthread_mutex_unlock(&pool->mutex);
        if (busy_flag) {
            pthread_cond_signal(&pool->without_busy_thread);
        }
    }
    printf("%lu -> sub thread exit\n", pthread_self());
    pthread_exit(NULL); // sub thread exit
}

void tiny_pool_boot(pool_t *pool) {
    /// pre-check to avoid invalid mutex waiting
    if (pool->status != PREPARING) {
        return; // only allow to boot at PREPARING stage
    }

    /// handle pool booting
    pthread_mutex_lock(&pool->mutex);
    if (pool->status != PREPARING) {
        pthread_mutex_unlock(&pool->mutex);
        return; // only allow to boot at PREPARING stage
    }
    pool->status = RUNNING;
    for (uint32_t i = 0; i < pool->thread_num; ++i) { // create working threads
        pthread_create(&(pool->threads[i]), NULL, thread_entry, (void*)pool);
    }
    printf("thread boot complete\n");
    pthread_mutex_unlock(&pool->mutex);
}

bool tiny_pool_join(pool_t *pool) {
    printf("start pool join\n");

    /// pre-check to avoid invalid mutex waiting
    if (pool->status != RUNNING) {
        return false;
    }

    /// handle pool threads joining
    pthread_mutex_lock(&pool->mutex);
    if (pool->status != RUNNING) {
        pthread_mutex_unlock(&pool->mutex);
        return false; // only allow to join at RUNNING stage
    }
    pool->status = STOPPING;
    printf("pool status -> STOPPING\n");

    printf("wait task queue\n");
    while (pool->task_queue_front != NULL) {
        pthread_cond_wait(&pool->task_queue_empty, &pool->mutex);
    }
    printf("task queue empty\n");

    pool->status = EXITING;
    printf("pool status -> EXITING\n");

    printf("start wait busy threads -> %d\n", pool->busy_thr_num);
    while (pool->busy_thr_num != 0) {
        pthread_cond_wait(&pool->without_busy_thread, &pool->mutex);
    }
    printf("all thread idle\n");

    pthread_mutex_unlock(&pool->mutex); // prevent other functions blocking waiting

    printf("start sub threads joining\n");
    for (uint32_t i = 0; i < pool->thread_num; ++i) {
        pthread_cond_broadcast(&pool->task_queue_not_empty); // trigger idle threads
        pthread_join(pool->threads[i], NULL);
        printf("sub thread %lu joined\n", pool->threads[i]);
    }
    printf("sub threads join complete\n");

    // TODO: free resource

    printf("start free pool resource\n");

    pthread_cond_destroy(&pool->without_busy_thread);
    pthread_cond_destroy(&pool->task_queue_not_empty);
    pthread_cond_destroy(&pool->task_queue_empty);
    pthread_mutex_destroy(&pool->mutex);

    free(pool->threads);
    free(pool);

    printf("free pool resource complete\n");

    return true;
}

void* run_pool_join(void *pool) {

    printf("run pool join from detach\n");

    tiny_pool_join((pool_t*)pool);

    printf("pool join complete\n");

    pthread_exit(NULL);
}

void tiny_pool_detach(pool_t *pool) {

    pthread_t tid;

    printf("run pool detach\n");

    pthread_create(&tid, NULL, run_pool_join, (void*)pool);

    pthread_detach(tid);

}
