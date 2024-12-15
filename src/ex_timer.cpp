#include "ex_platform.h"
#include <mmsystem.h>

void
ex::platform::timer::init(float target_seconds_per_frame) {
    m_target_seconds_per_frame = target_seconds_per_frame;
    QueryPerformanceFrequency(&m_counter_frequency);

    UINT desired_scheduler_ms = 1;
    m_sleep_is_granular = (timeBeginPeriod(desired_scheduler_ms) == TIMERR_NOERROR);
}

void
ex::platform::timer::start() {
    QueryPerformanceCounter(&m_last_counter);
    m_last_cycle_count = __rdtsc();
}

void
ex::platform::timer::end() {
    m_end_cycle_count = __rdtsc();
    QueryPerformanceCounter(&m_end_counter);

    uint64_t cycles_elapsed = m_end_cycle_count - m_last_cycle_count;
    int64_t counter_elapsed = m_end_counter.QuadPart - m_last_counter.QuadPart;

    m_seconds_elapsed = ((float)counter_elapsed / (float)m_counter_frequency.QuadPart);

    // float seconds_elapsed_for_frame = m_seconds_elapsed;
    // if (seconds_elapsed_for_frame < m_target_seconds_per_frame) {
    //     while (seconds_elapsed_for_frame < m_target_seconds_per_frame) {
    //         if (m_sleep_is_granular) {
    //             DWORD sleep_ms = (DWORD)(1000.0f * (m_target_seconds_per_frame - seconds_elapsed_for_frame));
    //             if (sleem_ms > 0) {
    //                 Sleep(sleep_ms);
    //             }
    //         }
            
    //         LARGE_INTEGER end_counter;
    //         QueryPerformanceCounter(&end_counter);
    //         seconds_elapsed_for_frame = ((float)(end_counter.QuadPart - m_last_counter.QuadPart) /
    //                                      (float)m_counter_frequency.QuadPart);
    //     }
    // } else {
    //     // TODO: MISSED FRAME RATE
    // }
    
    m_ms_per_frame = ((1000.0f * (double)counter_elapsed) / (double)m_counter_frequency.QuadPart);
    m_frames_per_second = (double)m_counter_frequency.QuadPart / (double)counter_elapsed;
    m_mega_cycles_per_frame = ((double)cycles_elapsed / (1000.0f * 1000.0f));

    m_last_counter = m_end_counter;
    m_last_cycle_count = m_end_cycle_count;
}
