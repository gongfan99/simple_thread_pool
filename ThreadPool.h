#pragma once

#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <future>
#include <vector>

class ThreadPool {
private:
  enum { running, stopping, stopped } status;
  std::queue<std::function<void()>> queue;
  std::vector<std::thread> worker_threads;

  std::mutex status_and_queue_mutex; // to lock both status and queue
  std::condition_variable status_and_queue_cv;

public:
  ThreadPool(const size_t n_threads) : worker_threads(std::vector<std::thread>(n_threads)), status(stopped) {}
  ThreadPool(const ThreadPool &) = delete;
  ThreadPool(ThreadPool &&) = delete;
  ThreadPool & operator=(const ThreadPool &) = delete;
  ThreadPool & operator=(ThreadPool &&) = delete;
  ~ThreadPool(){ shutdown(); }

  // Create threads in the pool and let them run
  void start() {
    for (auto &t : worker_threads) {
      t = std::thread([this](){
        std::function<void()> func;
        while (true) {
          {
            std::unique_lock<std::mutex> lock(status_and_queue_mutex);
            status_and_queue_cv.wait(lock, [this](){ return !queue.empty() || status == stopping; });
            if (status == stopping) break;
            func = std::move(queue.front());
            queue.pop();
          }
          func();
        }
      });
    }
    {
      std::unique_lock<std::mutex> lock(status_and_queue_mutex);
      status = running;
    }
  }

  // Waits until threads finish their current task and shutdowns the pool
  void shutdown() {
    {
      std::unique_lock<std::mutex> lock(status_and_queue_mutex);
      if (status != running) return;
      status = stopping;
    }
    status_and_queue_cv.notify_all();
    for (auto &t : worker_threads) { t.join(); }
    {
      std::unique_lock<std::mutex> lock(status_and_queue_mutex);
      status = stopped;
    }
  }

  // Submit a void (*)(...) function to be executed asynchronously by the pool
  // need to use std::ref to pass in reference
  template<typename F, typename...Args>
  void submit(F&& f, Args&&... args) {
    std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
    {
      std::unique_lock<std::mutex> lock(status_and_queue_mutex);
      queue.push(std::move(func));
    }
    status_and_queue_cv.notify_one();
  }
  
  // Submit any function
  // This is a more general form than "Submit" but may be slower???
  template<typename F, typename...Args>
  auto submitFuture(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
    auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(
      std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    std::function<void()> func = [task_ptr]() { (*task_ptr)(); };

    {
      std::unique_lock<std::mutex> lock(status_and_queue_mutex);
      queue.push(std::move(func));
    }
    status_and_queue_cv.notify_one();

    return task_ptr->get_future(); // calling thread will not wait at future destroyer
  }
};
