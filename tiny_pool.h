#ifndef TINY_POOL_H_
#define TINY_POOL_H_

#include <stdint.h>
#include <pthread.h>
#include <stdbool.h>

/// This is a lightweight thread pool designed for linux. It is implemented in C language. Its
/// goal is to provide simple and stable services, so it does not provide functions such as priority
/// and dynamic adjustment of the number of threads, and only provides the simplest threads pool
/// implementation.

/// The thread pool allows users to handle multiple tasks conveniently, and saves the additional
/// overhead of thread creation and destruction, which is very useful in tasks that require a large
/// number of short-term concurrent executions. There are four life cycle phases in this tiny thread
/// pool design:
///
///   + PREPARING: At this stage, the thread pool is in a ready state, and tasks can be added at
///                this time, but will not be run.
///
///   + RUNNING: The thread pool runs at this stage, all tasks will be assigned to each thread for
///              execution in sequence, and new tasks can still be added.
///
///   + STOPPING: The thread pool is about to be closed, and new tasks are no longer allowed at
///               this time, and the remaining tasks will be completed one by one.
///
///   + EXITING: At this point, there are no pending tasks, only working threads. When all threads
///              complete their tasks, the thread pool will be destroyed.
///
/// These four life cycles must proceed sequentially: PREPARING -> RUNNING -> STOPPING -> EXITING

/// When using it, you need to use `tiny_pool_create` to create a thread pool first, and then use
/// the `tiny_pool_submit` function to submit tasks to it. After the preparation is completed, use
/// `tiny_pool_boot` to start the operation of the thread pool, and then you can also add other tasks.

/// When preparing to exit, you have two options. The first is to use `tiny_pool_join`, which will
/// block and wait for all remaining tasks to be completed, then destroy the threads and free the
/// memory. The second is to use `tiny_pool_detach`, which will detach the thread pool and perform
/// the same behavior as the former after all tasks are completed. It is worth noting that after
/// running these two functions, no tasks can be added to this thread pool, and you should no longer
/// perform any operations on the thread pool.

/// In addition, there is a `tiny_pool_kill`, this command will directly clear all tasks, kill all
/// threads, all ongoing work will be canceled, it should only be used in emergency situations (such
/// as a fatal error in the main program). In other cases, it is recommended to use `tiny_pool_join`
/// or `tiny_pool_detach` interface.

enum pool_status {
    PREPARING,
    RUNNING,
    STOPPING,
    EXITING,
};

typedef struct task_t {
    void (*func)(void*); // function pointer of the task
    void *arg; // argument of task function
    struct task_t *next; // the next task in the queue
} task_t;

typedef struct pool_t {
    pthread_t *threads; // store thread id
    uint32_t thread_num; // number of threads

    enum pool_status status; // life cycle state
    pthread_mutex_t status_mutex; // mutex for `status`

    uint32_t busy_thr_num; // number of working threads
    pthread_mutex_t busy_thr_num_mutex; // mutex for `busy_thr_num`

    task_t *task_queue_front; // head of task queue
    task_t *task_queue_rear; // end of task queue
    uint32_t task_queue_size; // size of task queue
    pthread_mutex_t task_queue_busy; // mutex for `task_queue_xxx`

    pthread_cond_t task_queue_empty; // condition for task queue becomes empty
    pthread_cond_t task_queue_not_empty; // condition for task queue becomes not empty
} pool_t;

/// This function create a new thread pool, you need to specify the number of threads,
/// it will be in the `PREPARING` state, and return NULL on failure.
pool_t* tiny_pool_create(uint32_t size);

/// Submit a task to the specified thread pool, and the task is a function pointer (it
/// receives a void pointer and has no return value) and a parameter. It will return
/// true if the commit was successful, and false otherwise.
bool tiny_pool_submit(pool_t *pool, void (*func)(void*), void *arg);

/// This function will start the specified thread pool and change its state from
/// `PREPARING` to `RUNNING`. If the thread pool is already in non `PREPARING` at runtime,
/// it will have no effect.
void tiny_pool_boot(pool_t *pool);

/// This function will change the thread pool from `RUNNING` to `STOPPING`, and enter the
/// `EXITING` state when the queue is empty, likewise, if the state is non `RUNNING` when
/// entered, it will also have no effect. All tasks will automatically free resources after
/// completion. Note that it is blocking and may take a considerable amount of time.
void tiny_pool_join(pool_t *pool);

/// It is basically the same as `tiny_pool_join` in function, the difference is that it is
/// non-blocking, that is, it will automatically handle the remaining tasks and complete
/// resource free process after execution.
void tiny_pool_detach(pool_t *pool);

/// This function will forcibly clear the task queue and reclaim all resources. Note that
/// this will cause the interruption of running tasks and the loss of un-running tasks.
void tiny_pool_kill(pool_t *pool);

#endif
