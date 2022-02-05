#include "random_lcg.h"

LCG gRandomLCG;

void LCG::seed(u64 s) { state = s; }

void LCG::next() {
    state = 1103515245 * state + 12345;
}

u64 LCG::get() {
    next();
    return state;
}

u64 LCG::get(u64 max) {
    if (max == 0)
        return 0;
    next();
    return state % max;
}

u64 LCG::get(u64 min, u64 max) {
    if (min == 0 || max == 0)
        return get(max);
    if (min >= max)
        min = max - 1;
    next();
    return (state % (max - min)) + min;
}
