#include <queue>

#include "enclosure.h"

class Level {
public:
    int max_enclosures;
    std::deque<Enclosure> enclosures;
    Level(int max_enclosures) : max_enclosures(max_enclosures) {}
    // void dump(void) const;
    void empty(void);
};
