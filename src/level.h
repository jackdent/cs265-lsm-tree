#include <queue>

#include "enclosure.h"

class Level {
public:
    int max_enclosures;
    std::deque<Enclosure> enclosures;
    Level(int max_enclosures) : max_enclosures(max_enclosures) {}
    bool remaining(void) const {return max_enclosures - enclosures.size();}
    // void dump(void) const;
};
