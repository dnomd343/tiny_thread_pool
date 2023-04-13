# Tiny Thread Pool

> A lightweight thread pool for Linux.

This is a branch of packaging using c++, which provides more powerful functions and more convenient use. For the introduction of the underlying c, please refer to the `master` branch.

## Usage

The class `TinyPool` will maintain a thread pool lifecycle. The size of the thread pool can be specified in the constructor, and default is the number of CPU cores. If an error occurs, a `TinyPoolException` will be thrown.

The thread pool will not run after it is created, and it needs to call the `boot` function to start executing. Before and after boot, you can use the `submit` function to submit tasks to the thread pool.

When you are ready to exit, you can call the `join` or `detach` function. The former will block and wait for all threads to exit, and the latter will hand over the threads to the system management and exit until they are completed.

Another function `kill` is also provided, which will forcibly exit all threads, and unexecuted tasks will disappear, which should not be used under normal circumstances. By the way, if `TinyPool` is still running when it is destroyed, the kill process will be executed automatically.

You should note that `TinyPool` itself is thread-unsafe (although the content it manages is thread-safe), and you should not call its member functions separately in multiple threads, which will lead to unpredictable consequences.

## Compile

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

## Demo

Here provide a basic demo at `demo.cc`, you can try to compile and run it.

```bash
# static library
g++ demo.cc -L. -ltiny_pool -lpthread -o demo

# dynamic library
g++ demo.cc -L. -ltiny_pool -o demo
```

## License

MIT ©2023 [@dnomd343](https://github.com/dnomd343)
