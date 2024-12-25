#include "ex_platform.h"
#include <mmsystem.h>

void
ex::platform::timer::init() {
    QueryPerformanceFrequency(&m_counter_frequency);
}

uint64_t
ex::platform::timer::get_time() {
    LARGE_INTEGER out_counter;
    QueryPerformanceCounter(&out_counter);
    return out_counter.QuadPart;
}

uint64_t
ex::platform::timer::get_frequency() {
    return m_counter_frequency.QuadPart;
}
