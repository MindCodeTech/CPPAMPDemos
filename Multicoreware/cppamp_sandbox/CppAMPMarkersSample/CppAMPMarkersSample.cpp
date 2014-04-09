//RUN: false
//XFAIL: *
#include <amp.h>
#include <iostream>
#include <mutex>
#include <queue>
#include <future>
#include <thread>

using namespace concurrency;

// A helper data structure to communicate between the main and the secondary thread.
// It is assumed that there is one producer and one consumer.
struct shared_state_t {
  shared_state_t()
    : cancelled(false) {}

  void cancel() {
    std::unique_lock<std::mutex> _(m);
    cancelled = true;
    cv.notify_all();
  }

  bool is_cancelled() const {
    return cancelled;
  }

  void enqueue_marker(const std::shared_future<void>& marker) {
    std::unique_lock<std::mutex> _(m);
    queue.push(marker);
    cv.notify_all();
  }

  std::shared_future<void> dequeue_marker() {
    std::unique_lock<std::mutex> _(m);
    std::shared_future<void> marker = queue.front();
    queue.pop();
    return marker;
  }

  void wait() {
    std::unique_lock<std::mutex> lock(m);
    while(!cancelled && queue.empty()) {
      cv.wait(lock);
    }
  }

 private:
  std::mutex m;
  std::condition_variable cv;
  bool cancelled;
  std::queue<std::shared_future<void>> queue;
} shared_state;

void watch(const int progress_step) {
  int progress = 0;
  while(progress < 100) {
    // Wait for the change of the shared state.
    shared_state.wait();
    if(shared_state.is_cancelled()) {
    // Oops, something went wrong on the main thread.
      std::cout << "Cancelled!" << std::endl;
        break;
    }
    else {// !shared_state.queue.empty() == true
      // There is a marker for us.
      std::shared_future<void> marker = shared_state.dequeue_marker();
      // Wait for completition of the next parallel_for_each loop.
      marker.wait(); 
      // Communicate the progress to the user.
      progress += progress_step;
      std::cout << "Progress: " << progress << "%" << std::endl;
    }
  }
}

int main() {
  std::vector<float> data_host(1024);
  // ... Populate data_host ...

  // Note this value is tightly coupled with this specific algorithm.
  const int progress_step = 1;

  // Launch a secondary thread to monitor progress.
  std::thread watchdog;
  try {
    watchdog = std::thread(watch, progress_step);
  }
  catch(const std::system_error& e) {
    std::cerr << "Unable to start the watchdog thread: " << e.what() << std::endl;
    return 1;
  }

  try {
    accelerator_view acc_vw = accelerator().get_default_view();
    array<float, 1> data(data_host.size(), data_host.begin(), acc_vw);

    for(int i = 0; i < 100; i++) {
      // Perform pointless computation that would take some time in a non-optimized build.
      parallel_for_each(data.extent, [=, &data](index<1> idx) restrict(amp) {
        for(int j = 0; j < 100000; j++)
          data[idx] += static_cast<float>(idx[0]);
      });

    // Create a marker for 1% of progress.
      shared_state.enqueue_marker(acc_vw.create_marker());
      std::cout.flush();
    }

  // Main thread will be waiting here for the completition of all the parallel_for_each loops issued.
    copy(data, data_host.begin());
  }
  catch(const std::runtime_error&) {
  // Something went wrong, stop the watchdog.
  shared_state.cancel();

  // ... Additional error handling ...
  }

  watchdog.join();

  // ... Resume CPU processing of data_host ...
  std::cout << "Result[1]: " << data_host[1] << std::endl;
}
