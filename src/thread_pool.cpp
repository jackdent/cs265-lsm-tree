#include "thread_pool.h"

void ThreadPool::launch(function <void (void)>& func) {
    int i;

    for (i = 0; i < num_threads; i++) {
        workers.emplace_back(func);
    }
}

void ThreadPool::wait(void) {
    for (auto &worker : workers) {
        worker.join();
    }

    workers.clear();
}
