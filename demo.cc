#include <iostream>
#include <unistd.h>
#include "tiny_pool.h"

void demo_func(void *arg) {

    std::cout << *(int*)arg << std::endl;

    sleep(1);

}

int main() {
    std::cout << "tiny thread pool demo start" << std::endl;


    auto pool = tiny_pool_create(3);

    int dat[] = {0, 1, 2, 3, 4, 5, 6};
    tiny_pool_submit(pool, demo_func, (void*)&dat[0]);
    tiny_pool_submit(pool, demo_func, (void*)&dat[1]);
    tiny_pool_submit(pool, demo_func, (void*)&dat[2]);
    tiny_pool_submit(pool, demo_func, (void*)&dat[3]);
    tiny_pool_submit(pool, demo_func, (void*)&dat[4]);
    tiny_pool_submit(pool, demo_func, (void*)&dat[5]);
    tiny_pool_submit(pool, demo_func, (void*)&dat[6]);

    tiny_pool_boot(pool);

    tiny_pool_join(pool);

    std::cout << "tiny thread pool demo exit" << std::endl;
    return 0;
}
