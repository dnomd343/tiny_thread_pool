#include <iostream>
#include <unistd.h>
#include "tiny_pool.h"

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
