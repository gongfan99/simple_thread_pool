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

  // Inits thread pool
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
      status = stopping;
    }
    status_and_queue_cv.notify_all();
    for (auto &t : worker_threads) { t.join(); }
    {
      std::unique_lock<std::mutex> lock(status_and_queue_mutex);
      status = stopped;
    }
  }

  // Submit a function to be executed asynchronously by the pool
  // need to use std::ref to pass in reference
/*   template<typename F, typename...Args>
  void submit(F&& f, Args&&... args) {
    std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
    {
      std::unique_lock<std::mutex> lock(status_and_queue_mutex);
      queue.push(func);
    }
    status_and_queue_cv.notify_one();
  } */
  
  template<typename F, typename...Args>
  auto submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
/*     std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
    std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);
    std::packaged_task<decltype(f(args...))()> task(std::bind(std::forward<F>(f), std::forward<Args>(args)...)); */
    /* std::promise<decltype(f(args...))> p; */
    std::promise<decltype(f(args...))> p;
    /* std::future<decltype(f(args...))> ft = p.get_future(); */
    std::future<decltype(f(args...))> ft = p.get_future();
/*     std::function<void()> func = std::bind([](F&& f, Args&&... args, std::promise<decltype(f(args...))>& pp){
      pp.set_value(f(args...));
    }, std::forward<F>(f), std::forward<Args>(args)..., std::move(p)); */
/*     std::function<void()> func = std::bind([](std::promise<decltype(f(args...))> pp){
    }, std::move(p)); */
    std::function<void()> func = [](){};
    {
      std::unique_lock<std::mutex> lock(status_and_queue_mutex);
      queue.push(std::move(func));
    }
    status_and_queue_cv.notify_one();
    return ft;
  }
};
