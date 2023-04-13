#pragma once

#include <future>
#include <stdexcept>
#include <functional>
#include "tiny_pool/tiny_pool.h"

class TinyPoolException : public std::runtime_error {
public:
    explicit TinyPoolException(const std::string &msg) : std::runtime_error(msg) {}
    ~TinyPoolException() noexcept override = default;
};

class TinyPool { // OOP for tiny_thread_pool

    pool_t *pool_ = nullptr; // thread pool c-api entry

    static pool_t* create_pool(uint32_t size) {
        auto pool = tiny_pool_create(size);
        if (pool == nullptr) {
            throw TinyPoolException("tiny thread pool create failed");
        }
        return pool;
    }

    pool_t* transfer_pool() {
        if (pool_ == nullptr) {
            throw TinyPoolException("tiny thread pull already destroyed");
        }
        auto pool = pool_;
        pool_ = nullptr; // clear class member
        return pool;
    }

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

    /// Create new thread pool.
    TinyPool() { pool_ = create_pool(cpu_core_num()); }
    explicit TinyPool(uint32_t size) { pool_ = create_pool(size); }

    /// Start running the thread pool.
    void boot() {
        if (pool_ == nullptr) {
            throw TinyPoolException("tiny thread pull already destroyed");
        }
        tiny_pool_boot(pool_);
    }

    /// Join all threads.
    void join() { tiny_pool_join(transfer_pool()); }

    /// Force quit all threads.
    void kill() { tiny_pool_kill(transfer_pool()); }

    /// Detach all threads.
    void detach() { tiny_pool_detach(transfer_pool()); }

    /// Submit new task to the thread pool.
    template <typename Func, typename ...Args>
    auto submit(Func &&func, Args &&...args) -> std::future<decltype(func(args...))> {
        using return_type = decltype(func(args...)); // declare function return type
        auto func_ptr = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<Func>(func), std::forward<Args>(args)...) // wrap as a function without params
        );
        tiny_pool_submit(pool_, TinyPool::wrap_c_func, (void*)( // using thread pool interface for submit
            new std::function<void()> (
                [func_ptr]() { (*func_ptr)(); } // wrapped as lambda for running task
            )
        ));
        return func_ptr->get_future(); // return future object
    }

    ~TinyPool() {
        if (pool_ != nullptr) { kill(); }
    }
};
