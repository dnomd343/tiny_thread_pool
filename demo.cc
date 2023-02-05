#include <future>
#include <functional>
#include "tiny_pool.h"

class TinyPool {
    pool_t *pool;
    static void wrap_c_func(void *func) {
        (*static_cast<std::function<void()>*>(func))();
        free(func);
    }

public:
    void boot() { tiny_pool_boot(pool); }
    void join() { tiny_pool_join(pool); }
    void kill() { tiny_pool_kill(pool); }
    void detach() { tiny_pool_detach(pool); }
    explicit TinyPool(uint32_t size) { pool = tiny_pool_create(size); }

    template <typename Func, typename ...Args>
    auto submit(Func &&func, Args &&...args) -> std::future<decltype(func(args...))>;
};


template <typename Func, typename ...Args>
auto TinyPool::submit(Func &&func, Args &&...args) -> std::future<decltype(func(args...))> {
    std::function<decltype(func(args...))()> wrap_func = std::bind(
        std::forward<Func>(func), std::forward<Args>(args)...
    );
    auto func_ptr = std::make_shared<
        std::packaged_task<decltype(func(args...))()>
    >(wrap_func);
    tiny_pool_submit(pool, TinyPool::wrap_c_func, (void*)(
        new std::function<void()> (
            [func_ptr]() { (*func_ptr)(); }
        )
    ));
    return func_ptr->get_future();
}

/// -------------------------------- start test --------------------------------

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
    std::cout << "tiny thread pool demo start" << std::endl;

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

    std::cout << "tiny thread pool demo exit" << std::endl;
    return 0;
}
