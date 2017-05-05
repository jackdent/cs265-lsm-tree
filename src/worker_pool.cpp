#include <cassert>

#include "worker_pool.h"

void WorkerPool::launch(worker_task& task) {
    assert(futures.size() == 0);

    for (int i = 0; i < workers.size(); i++) {
        futures.push_back(enqueue(task));
    }
}

void WorkerPool::wait_all(void) {
    for (auto& future : futures) {
        future.wait();
    }

    futures.clear();
}

void DynamicWorkerPool::launch(worker_task& task) {
    for (int i = 0; i < workers.size(); i++) {
        workers.emplace_back(task);
    }
}

void DynamicWorkerPool::wait_all(void) {
    for (auto& worker : workers) {
        worker.join();
    }
}
