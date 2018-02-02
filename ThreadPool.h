#pragma once

#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool {
private:
  enum { running, stopping, stopped } status;
  std::queue<std::function<void()>> queue;
  std::vector<std::thread> m_threads;

  std::mutex status_and_queue_mutex; // to lock both status and queue
  std::condition_variable status_and_queue_cv;

public:
  ThreadPool(const int n_threads) : m_threads(std::vector<std::thread>(n_threads)), status(stopped) {}
  ThreadPool(const ThreadPool &) = delete;
  ThreadPool(ThreadPool &&) = delete;
  ThreadPool & operator=(const ThreadPool &) = delete;
  ThreadPool & operator=(ThreadPool &&) = delete;

  // Inits thread pool
  void start() {
    for (auto &t : m_threads) {
      t = std::thread([this](){
        std::function<void()> func(nullptr);
        while (true) {
          {
            std::unique_lock<std::mutex> lock(status_and_queue_mutex);
            status_and_queue_cv.wait(lock, [this](){ return !queue.empty() || status == stopping; });
            if (!queue.empty()){
              func = std::move(queue.front());
              queue.pop();
            }
            if (status == stopping) break;
          }
          if (func) func();
          func = nullptr;
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
    for (auto &t : m_threads) { t.join(); }
    {
      std::unique_lock<std::mutex> lock(status_and_queue_mutex);
      status = stopped;
    }
  }

  // Submit a function to be executed asynchronously by the pool
  // need to use std::ref to pass in reference
  template<typename F, typename...Args>
  void submit(F&& f, Args&&... args) {
    std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
    {
      std::unique_lock<std::mutex> lock(status_and_queue_mutex);
      queue.push(func);
    }
    status_and_queue_cv.notify_one();
  }
};
