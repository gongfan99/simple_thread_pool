# simple_thread_pool
A bare metal thread pool with C++11. It should work on both Linux and Windows.

# build example
```c
// Windows, VS 2015 installed
git clone https://github.com/gongfan99/simple_thread_pool.git
cd test
build
test
```

# usage
```c
// Create pool with 3 threads
ThreadPool pool(3);

// Create threads in the pool and let them run
pool.start();

// Submit work
// lambda function is used here as example. Any callable type can be used. But return value has to be void.
pool.submit( [](float data) { process(data); }, 1.234 );

// Submit work
// the return value is ignored
pool.submitFuture( [](float data) -> float { return process(data); }, 1.234 );

// Submit work and get future associated with the result
auto fut = pool.submitFuture( [](float data) -> float { return process(data); }, 1.234 );
assert( fut.get() == process(1.234) );

// Shutdown the pool, releasing all threads
pool.shutdown();
```

More usage cases can be found in test/test.cc

Either submit() or submitFuture() can be used. submit() may be slightly faster because of less wrapping but only funtion that returns void can be submitted. submitFuture() supports any form of function but may be slower.

# further speedup
You can come up with an even faster version if the function to be submitted has a fixed known form for example "void func(int)", then the slow std::bind can be removed from the submit() implementation.

First define a class
```c
class FunctionWrapper {
  void (*)(int) pFunc;
  int input;
  FunctionWrapper(void (*)(int) pF, int inp) : pFunc(pF), input(inp) {}
}
```

Then the queue in the class ThreadPool needs to be modified to:
```c
std::queue<FunctionWrapper> queue;
```

submit() can be modified to:
```c
void submit(void (*)(int) pF, int inp) {
  {
    std::unique_lock<std::mutex> lock(status_and_queue_mutex);
    queue.emplace(pF, inp);
  }
  status_and_queue_cv.notify_one();
}
```

Finally in the start() of the class ThreadPool, the function invocation needs to be modified from func() to:
```c
(func -> pFunc)(func -> input);
```

# Reference
This implementation is adapted from [Mtrebi's thread pool](https://github.com/mtrebi/thread-pool) which has a very good description.