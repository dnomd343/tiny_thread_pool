# Tiny Thread Pool

> A lightweight thread pool for Linux.

This is a lightweight thread pool designed for Linux, it is implemented in C language. Its goal is to provide simple and stable services, so it does not provide functions such as priority and dynamic adjustment of the number of threads, and only provides the simplest threads pool implementation.

The thread pool allows users to handle multiple tasks conveniently, and saves the additional overhead of thread creation and destruction, which is very useful in tasks that require a large number of short-term concurrent executions. There are four life cycle phases in this tiny thread pool design:

+ `PREPARING`: At this stage, the thread pool is in a ready state, and tasks can be added at this time, but will not be run.

+ `RUNNING`: The thread pool runs at this stage, all tasks will be assigned to each thread for execution in sequence, and new tasks can still be added.

+ `STOPPING`: The thread pool is about to be closed, and new tasks are no longer allowed at this time, and the remaining tasks will be completed one by one.

+ `EXITING`: At this point, there are no pending tasks, only working threads. When all threads complete their tasks, the thread pool will be destroyed.

These four life cycles must proceed sequentially: `PREPARING` `->` `RUNNING` `->` `STOPPING` `->` `EXITING`

> NOTE: The STOPPING and EXITING states are automatically managed by the thread pool, and users do not need to care about them.

## Usage

When using it, you need to use `tiny_pool_create` to create a thread pool first, and then use the `tiny_pool_submit` function to submit tasks to it. After the preparation is completed, use `tiny_pool_boot` to start the operation of the thread pool, and then you can also add other tasks.

When preparing to exit, you have two options. The first is to use `tiny_pool_join`, which will block and wait for all remaining tasks to be completed, then destroy the threads and free the memory. The second is to use `tiny_pool_detach`, which will detach the thread pool and perform the same behavior as the former after all tasks are completed. It is worth noting that after running these two functions, no tasks can be added to this thread pool, and you should no longer perform any operations on the thread pool.

In addition, there is a `tiny_pool_kill`, this command will directly clear all tasks, kill all threads, all ongoing work will be canceled, it should only be used in emergency situations (such as a fatal error in the main program). In other cases, it is recommended to use `tiny_pool_join` or `tiny_pool_detach` interface.

## Demo

This is a basic c-based use, file at `demo/c/demo.c`:

```c
#include <stdio.h>
#include <unistd.h>
#include "tiny_pool.h"

void test_fun(void *i) {
    int num = *(int*)i;
    printf("task %d start\n", num);
    for (int t = 0; t < num; ++t) {
        sleep(1);
        printf("task %d running...\n", num);
    }
    printf("task %d complete\n", num);
}

int main() {
    int dat[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};

    pool_t *pool = tiny_pool_create(4);

    tiny_pool_submit(pool, test_fun, (void*)&dat[0]);
    tiny_pool_submit(pool, test_fun, (void*)&dat[1]);

    tiny_pool_boot(pool);

    sleep(5);

    tiny_pool_submit(pool, test_fun, (void*)&dat[2]);
    tiny_pool_submit(pool, test_fun, (void*)&dat[3]);
    tiny_pool_submit(pool, test_fun, (void*)&dat[4]);
    tiny_pool_submit(pool, test_fun, (void*)&dat[5]);
    tiny_pool_submit(pool, test_fun, (void*)&dat[6]);

    sleep(6);

    tiny_pool_submit(pool, test_fun, (void*)&dat[7]);
    tiny_pool_submit(pool, test_fun, (void*)&dat[8]);

    tiny_pool_join(pool);

    return 0;
}
```

In C++, the C ABI interface can be called directly. Here, OOP is used to repackage, which provides stronger functions under C++11, file at `demo/cpp/demo.cc`:

```cpp
#include <future>
#include <functional>
#include "tiny_pool.h"

class TinyPool { // OOP for tiny_thread_pool
    pool_t *pool;
    static void wrap_c_func(void *func) { // wrap std::function as c-style function ptr
        auto func_ptr = static_cast<std::function<void()>*>(func);
        (*func_ptr)();
        delete func_ptr; // free lambda function
    }

public:
    void boot() { tiny_pool_boot(pool); }
    void join() { tiny_pool_join(pool); }
    void kill() { tiny_pool_kill(pool); }
    void detach() { tiny_pool_detach(pool); }
    explicit TinyPool(uint32_t size) { pool = tiny_pool_create(size); }

    template <typename Func, typename ...Args>
    auto submit(Func &&func, Args &&...args) -> std::future<decltype(func(args...))> { // submit new task
        std::function<decltype(func(args...))()> wrap_func = std::bind(
            std::forward<Func>(func), std::forward<Args>(args)... // wrap as a function without params
        );
        auto func_ptr = std::make_shared<
            std::packaged_task<decltype(func(args...))()> // function task as shared ptr
        >(wrap_func);
        tiny_pool_submit(pool, TinyPool::wrap_c_func, (void*)( // submit with thread pool interface
            new std::function<void()> (
                [func_ptr]() { (*func_ptr)(); } // create lambda for running task
            )
        ));
        return func_ptr->get_future(); // return future object
    }
};

/// ------------------------------------ start test ------------------------------------

#include <iostream>
#include <unistd.h>

int test_func(char c) {
    int num = c - '0';
    printf("char -> `%c`\n", c);
    for (int i = 0; i < num; ++i) {
        printf("task %d running...\n", num);
        usleep(500 * 1000);
    }
    return num;
}

int main() {
    auto pool = TinyPool(3);

    auto f0 = pool.submit(test_func, '0');
    auto f1 = pool.submit(test_func, '1');
    auto f2 = pool.submit(test_func, '2');
    auto f3 = pool.submit(test_func, '3');

    pool.boot();

    auto f4 = pool.submit(test_func, '4');
    auto f5 = pool.submit(test_func, '5');

    printf("get future: %d\n", f0.get());
    printf("get future: %d\n", f4.get());
    printf("get future: %d\n", f3.get());

    auto f6 = pool.submit(test_func, '6');
    auto f7 = pool.submit(test_func, '7');
    auto f8 = pool.submit(test_func, '8');
    auto f9 = pool.submit(test_func, '9');

    printf("get future: %d\n", f2.get());
    printf("get future: %d\n", f5.get());
    printf("get future: %d\n", f8.get());

    pool.join();

    printf("get future: %d\n", f6.get());
    printf("get future: %d\n", f1.get());
    printf("get future: %d\n", f9.get());
    printf("get future: %d\n", f7.get());
}
```

## Compile

This project uses `CMake` to build and compile, you can use it to get started quickly, or manually call the compiler.

**CMake**

> This will build `libtiny_pool.a` static library and demo executable.

```bash
cmake -B cmake-build
cmake --build cmake-build
```

> This will build `libtiny_pool.so` dynamic library and demo executable.

```bash
cmake -B cmake-build -DBUILD_DYN=ON
cmake --build cmake-build
```

**GCC**

> Manually build the static library.

```bash
gcc -std=gnu99 -c tiny_pool.c -o libtiny_pool.a
```

> Build the demo and link the static library.

```bash
gcc demo/c/demo.c -o demo_c -I. -L. -ltiny_pool -lpthread
g++ demo/cpp/demo.cc -o demo_cpp -I. -L. -ltiny_pool -lpthread
```

> Manually build the dynamic library.

```bash
gcc -std=gnu99 -shared -fPIC -c tiny_pool.c -o libtiny_pool.so
```

## License

MIT ©2023 [@dnomd343](https://github.com/dnomd343)
