#include <functional>
#include <thread>
#include <vector>

using namespace std;

typedef function<void(void)> worker_task;

class ThreadPool {
    mutex main_mtx;
    condition_variable main_cv;
    mutex thread_mtx;
    condition_variable thread_cv;
    vector<thread> workers;
    worker_task *task;
    atomic<int> completed;
    bool stop;
public:
    ThreadPool(int);
    ~ThreadPool(void);
    void launch(worker_task&);
    void wait_all(void);
};
