#include <queue>

#include "run.h"

class Level {
public:
    int max_runs;
    long max_run_size;
    std::deque<Run> runs;
    Level(int n, long s) : max_runs(n), max_run_size(s) {}
    bool remaining(void) const {return max_runs - runs.size();}
};
