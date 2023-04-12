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
