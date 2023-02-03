#include <stdio.h>
#include <unistd.h>
#include "tiny_pool.h"

void demo_fun(void *i) {
    int num = *(int*)i;
    printf("task %d start\n", num);
    for (int t = 0; t < num; ++t) {
        sleep(1);
        printf("task %d running...\n", num);
    }
    printf("task %d complete\n", num);
}

int main() {

    pool_t *pool = tiny_pool_create(2);

    int dat[] = {1, 2, 3, 4, 5, 6, 7, 8};
    tiny_pool_submit(pool, demo_fun, (void*)&dat[0]);
    tiny_pool_submit(pool, demo_fun, (void*)&dat[1]);

    printf("main: pool booting\n");
    tiny_pool_boot(pool);
    printf("main: pool boot complete\n");

    printf("main: sleep 5s\n");
    sleep(5);
    printf("main: wake up\n");

    tiny_pool_submit(pool, demo_fun, (void*)&dat[2]);
    tiny_pool_submit(pool, demo_fun, (void*)&dat[3]);
    tiny_pool_submit(pool, demo_fun, (void*)&dat[4]);

    printf("main: sleep 8s\n");
    sleep(6);
    printf("main: wake up\n");

    // TODO: tiny pool join

//    printf("pool try exit\n");
//    tiny_pool_kill(pool);

    // TODO: tiny pool destroy

    sleep(10);

    printf("main exit\n");

    return 0;
}
