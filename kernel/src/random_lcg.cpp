#include "random_lcg.h"

LCG gRandomLCG;

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
