#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "tiny_pool.h"

void demo_fun(void *i) {

    int k = *(int*)i;

    printf("demo func sleep %ds\n", k);

    sleep(k);

    printf("demo func %d wake up\n", k);

}

int main() {

//    pthread_t tid;

    pool_t *pool = tiny_pool_create(2);

    int dat[] = {1, 2, 3, 4, 5, 6, 7, 8};
    tiny_pool_submit(pool, demo_fun, (void*)&dat[0]);
    tiny_pool_submit(pool, demo_fun, (void*)&dat[1]);

    printf("pool booting\n");
    tiny_pool_boot(pool);
    printf("pool running\n");

    printf("main thread sleep\n");
    sleep(5);
    printf("main thread wake up\n");

    tiny_pool_submit(pool, demo_fun, (void*)&dat[2]);
    tiny_pool_submit(pool, demo_fun, (void*)&dat[3]);
    tiny_pool_submit(pool, demo_fun, (void*)&dat[4]);

    sleep(8);

    // TODO: tiny pool join

    printf("pool try exit\n");
    tiny_pool_kill(pool);

    // TODO: tiny pool destroy

    sleep(20);

    printf("main exit\n");

    return 0;
}
