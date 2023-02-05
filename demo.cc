#include <iostream>
#include <stdexcept>
#include "tiny_pool.h"

#include <unistd.h>
#include <future>
#include <functional>

void demo_func(void *arg) {

    std::cout << *(int*)arg << std::endl;

    sleep(1);

}

int test_func(char c) {


    printf("char -> `%c`\n", c);

    return 233;

}


void convert(void *f) {

//    auto *func = ( std::function<void()> * ) f;
//
//    (*func)();

    ( *static_cast< std::function<void()>* >(f) )();

//    auto t = (std::function<void(void*)>*)task;
//
//    (*t)(NULL);

    free(f);

}

class TinyPool {
    pool_t *pool;
public:

    TinyPool(uint32_t size);

    ~TinyPool();


    template <typename Func, typename ...Args>
    auto submit(Func &&func, Args &&...args) -> std::future<decltype(func(args...))> {

        std::function<decltype(func(args...))()> wrap_func = std::bind(
            std::forward<Func>(func), std::forward<Args>(args)...
        );

        auto func_ptr = std::make_shared<
            std::packaged_task<decltype(func(args...))()>
        >(wrap_func);

//        std::function<void(void*)> task_func = [func_ptr](void*) {
//            (*func_ptr)();
//        };

        auto *t = new std::function<void()>;
        *t = [func_ptr](){
            (*func_ptr)();
        };


        // TODO: run task_func

//        (*t)();

//        convert( (void*)t );

//        auto t = reinterpret_cast<void(void*)>(task_func);

        tiny_pool_submit(pool, convert, (void*)t);

        return func_ptr->get_future();

    }


    void boot() {
        tiny_pool_boot(pool);
    }

};





TinyPool::TinyPool(uint32_t size) {

    pool = tiny_pool_create(size);

//    if (pool == (void*)0) { // NULL in c-style
//        throw std::runtime_error("thread pool create error");
//    }

}

TinyPool::~TinyPool() {
    tiny_pool_kill(pool);
}

int main() {
    std::cout << "tiny thread pool demo start" << std::endl;

    auto p = TinyPool(1);

    auto f = p.submit(test_func, 'A');

    p.boot();

    std::cout << "get future: " << f.get() << std::endl;


//    auto f = submit(test_func, 'D');

//    auto pool = tiny_pool_create(3);

//    int dat[] = {0, 1, 2, 3, 4, 5, 6};
//    tiny_pool_submit(pool, demo_func, (void*)&dat[0]);
//    tiny_pool_submit(pool, demo_func, (void*)&dat[1]);
//    tiny_pool_submit(pool, demo_func, (void*)&dat[2]);
//    tiny_pool_submit(pool, demo_func, (void*)&dat[3]);
//    tiny_pool_submit(pool, demo_func, (void*)&dat[4]);
//    tiny_pool_submit(pool, demo_func, (void*)&dat[5]);
//    tiny_pool_submit(pool, demo_func, (void*)&dat[6]);
//
//    tiny_pool_boot(pool);
//
//    tiny_pool_join(pool);

    sleep(10);

    std::cout << "tiny thread pool demo exit" << std::endl;
    return 0;
}
