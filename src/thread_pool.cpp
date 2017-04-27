#include <cassert>

#include "thread_pool.h"

ThreadPool::ThreadPool(int num_threads) : stop(false) {
    while ((num_threads--) > 0) {
        workers.emplace_back([&] {
            for (;;) {
                unique_lock<mutex> lock(thread_mtx);
                thread_cv.wait(lock);

                if (stop) break;

                (*task)();
                completed++;

                if (completed == workers.size()) {
                    unique_lock<mutex> lock(main_mtx);
                    main_cv.notify_one();
                }
            }
        });
    }
}

ThreadPool::~ThreadPool(void) {
    stop = true;

    unique_lock<mutex> lock(thread_mtx);
    thread_cv.notify_all();

    for (auto &worker : workers) worker.join();
    workers.clear();
}

void ThreadPool::launch(worker_task& fn) {
    completed = 0;
    task = &fn;

    unique_lock<mutex> lock(thread_mtx);
    thread_cv.notify_all();
}

void ThreadPool::wait_all(void) {
    unique_lock<mutex> lock(main_mtx);
    main_cv.wait(lock, [&] {return completed == workers.size();});
}
