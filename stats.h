// stats.h
#pragma once
#include <cstdint>

struct VMStat {
    uint64_t totalMemory;
    uint64_t usedMemory;
    uint64_t freeMemory;

    uint64_t idleCpuTicks;
    uint64_t activeCpuTicks;
    uint64_t totalCpuTicks;

    uint64_t pagedIn;
    uint64_t pagedOut;
};
