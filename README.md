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

## Compile

This project uses `CMake` to build and compile, you can use it to get started quickly, or manually call the compiler.

**CMake**

The following commands will build `libtiny_pool.a` static library.

```bash
cmake -B cmake-build
cmake --build cmake-build
```

This commands will build `libtiny_pool.so` dynamic library.

```bash
cmake -B cmake-build -DBUILD_DYN=ON
cmake --build cmake-build
```

**GCC**

You can also build the static library manually.

```bash
gcc -O3 -DNDEBUG -std=gnu99 -c tiny_pool.c -o libtiny_pool.a
```

And this command will build the dynamic library.

```bash
gcc -O3 -DNDEBUG -fPIC -std=gnu99 -c tiny_pool.c -o tiny_pool.o
gcc -O3 -DNDEBUG -fPIC -shared tiny_pool.o -lpthread -o libtiny_pool.so
```

## Demo

Here provide a basic c-based use at `demo.c`, you can try to compile and run it.

```bash
# static library
gcc demo.c -L. -ltiny_pool -lpthread -o demo

# dynamic library
gcc demo.c -L. -ltiny_pool -o demo
```

## License

MIT ©2023 [@dnomd343](https://github.com/dnomd343)
