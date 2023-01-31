#include <stdio.h>
#include <pthread.h>
#include "tiny_pool.h"

void* demo_fun(void *i) {
    printf("demo func -> %d\n", *(int*)i);
    return NULL;
}

int main() {

    pthread_t tid;

    int d = 123;

    pool_t *pool = tiny_pool_create(0);

    tiny_pool_submit(pool, demo_fun, (void*)&d);



    return 0;
}
