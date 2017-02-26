//
// Created by Martin Wickham on 10/25/2016.
//

#ifndef PERF_H
#define PERF_H

#ifdef WINDOWS
#include <afxres.h>

void initPerformanceData();
void printPerformanceData();
void recordPerformanceData(const char *name, const LONGLONG timeElapsed);
void markPerformanceFrame();

class Perf {
private:
    const char * const name;
    LARGE_INTEGER startTime;

public:
    Perf(const char *name) :
            name(name)
    {
        QueryPerformanceCounter(&startTime);
    }

    ~Perf() {
#ifdef PERF
        LARGE_INTEGER endTime;
        QueryPerformanceCounter(&endTime);
        recordPerformanceData(name, endTime.QuadPart - startTime.QuadPart);
#endif
    }
};

#else
inline void initPerformanceData() {}
inline void printPerformanceData() {}
inline void markPerformanceFrame() {}
#define recordPerformanceData(name, time) do {sizeof(name); sizeof(time);} while(0)

struct Perf {
    Perf(const char *name) {}
};
#endif

#endif //STUPIDSHTRICKS_PERF_H
