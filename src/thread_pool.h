#include <functional>
#include <thread>
#include <vector>

using namespace std;

class ThreadPool {
    vector<thread> workers;
    int num_threads;
public:
    ThreadPool(int num_threads) : num_threads(num_threads) {}
    void launch(function <void (void)>&);
    void wait(void);
};
