# Simple_thread_pool
A bare metal thread pool with C++11. It works on both Linux and Windows.

# Build example
```CPP
// on Windows with VS 2015 installed
git clone https://github.com/gongfan99/simple_thread_pool.git
cd test
build
test
```

# Usage
```CPP
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

// Shutdown the pool, release all threads
pool.shutdown();
```

More usage cases can be found in test/test.cc

Either `submit()` or `submitFuture()` can be used. `submit()` may be slightly faster but only funtion that returns void can be submitted. `submitFuture()` supports any form of function but may be slower due to the `future` involved.

`submit()` is very good for writing Node native module.

# Further speedup
If the function to be submitted has a fixed known signature for example `void func(int)`, then the slow `std::bind` can be removed from the `submit()` implementation. `std::bind` is slow because of two reasons: (a) indirect function invoking (b) possible heap allocation if the function argument size is larger than one pointer size.

However, you should not do this unless you are absolutely sure this speedup is needed. It is not in most cases.

1. First define a class
```CPP
class FunctionWrapper {
  typedef void (*FuncMemberType)(int);
  FuncMemberType pFunc;
  int input;
  FunctionWrapper(FuncMemberType pF, int inp) : pFunc(pF), input(inp) {}
  void operator()() { pFunc(input); }
}
```

2. Modify the `queue` in the `class ThreadPool` to:
```CPP
std::queue<FunctionWrapper> queue;
```

3. Modify `submit()` to:
```CPP
void submit(FuncMemberType pF, int inp) {
  {
    std::unique_lock<std::mutex> lock(status_and_queue_mutex);
    queue.emplace(pF, inp);
  }
  status_and_queue_cv.notify_one();
}
```

# Reference
This implementation is adapted from [Mtrebi's thread pool](https://github.com/mtrebi/thread-pool) which has a very good description of the code.

# License
MIT
