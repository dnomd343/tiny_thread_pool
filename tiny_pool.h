#pragma once

#include <future>
#include <functional>
#include "tiny_pool/tiny_pool.h"

class TinyPool { // OOP for tiny_thread_pool

    pool_t *pool; // thread pool c-api entry

    static void wrap_c_func(void *func) { // wrap std::function as c-style function ptr
        auto func_ptr = static_cast<std::function<void()>*>(func);
        (*func_ptr)();
        delete func_ptr; // free lambda function
    }

public:
    /// Get the number of CPU cores.
    static uint32_t cpu_core_num() {
        auto num = std::thread::hardware_concurrency(); // CPU core number
        return (num == 0) ? 1 : num; // return 1 when fetch failed
    }

    /// Create thread pool of a specified size.
    explicit TinyPool(uint32_t size) {
        pool = tiny_pool_create(size);
    }

    // TODO: free thread pool when failed
    void boot() { tiny_pool_boot(pool); }
    void join() { tiny_pool_join(pool); }
    void kill() { tiny_pool_kill(pool); }
    void detach() { tiny_pool_detach(pool); }

    /// Submit new task to the thread pool.
    template <typename Func, typename ...Args>
    auto submit(Func &&func, Args &&...args) -> std::future<decltype(func(args...))> {
        using return_type = decltype(func(args...)); // declare function return type
        auto func_ptr = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<Func>(func), std::forward<Args>(args)...) // wrap as a function without params
        );
        tiny_pool_submit(pool, TinyPool::wrap_c_func, (void*)( // using thread pool interface for submit
            new std::function<void()> (
                [func_ptr]() { (*func_ptr)(); } // wrapped as lambda for running task
            )
        ));
        return func_ptr->get_future(); // return future object
    }
};
