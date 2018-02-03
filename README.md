# simple_thread_pool
A bare metal thread pool with C++11. It should work on both Linux and Windows.

# Usage example (Windows)
git clone https://github.com/gongfan99/simple_thread_pool.git
cd test
build
test

Note: either submit() or submitFuture() can be used. I guess submit() is faster because of less wrapping but it only supports submitting void (*)(...) form of function. submitFuture() supports any form of function but it could be slower because of more wrapping. You can come up with a even faster version of submit() if the function to be submitted has a fixed form for example int f(int), then std::bind is not needed in the code. 

# Reference
It is modified based on
https://github.com/mtrebi/thread-pool