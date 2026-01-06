#pragma once
#include <x86intrin.h>
#include <cstdint>
#include <thread>
#include <iostream>
#include <cstring>
#include <pthread.h> 
#include <sched.h> 
#include <time.h>
#include <algorithm>
#include <x86intrin.h>

int cpu_count = std::thread::hardware_concurrency();

void pin_thread_to_core(int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);

    core_id = core_id%cpu_count;
    CPU_SET(core_id, &cpuset);   // wrap around

    int rc = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if (rc != 0) std::cerr << "affinity failed: " << strerror(rc) << "\n";
}

inline uint64_t rdtsc_now() {
    unsigned int aux;
    return __rdtscp(&aux);
}

static inline uint64_t monotonic_raw_ns() {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return uint64_t(ts.tv_sec) * 1'000'000'000ull + ts.tv_nsec;
}

double calibrate_ghz() {
    // pin calibration thread
    pin_thread_to_core(0);

    constexpr int samples = 5;
    constexpr uint64_t duration_ns = 200'000'000; // 200 ms

    double freqs[samples];

    for (int i = 0; i < samples; ++i) {
        uint64_t t0_ns = monotonic_raw_ns();
        uint64_t c0 = rdtsc_now();

        // busy wait instead of sleep
        while (monotonic_raw_ns() - t0_ns < duration_ns) {
            _mm_pause();
        }

        uint64_t t1_ns = monotonic_raw_ns();
        uint64_t c1 = rdtsc_now();

        double elapsed_s = double(t1_ns - t0_ns) * 1e-9;
        freqs[i] = double(c1 - c0) / elapsed_s / 1e9;
    }

    std::sort(freqs, freqs + samples);
    return freqs[samples / 2]; // median
}

double cycles_to_ns(uint64_t cycles, double ghz) {
    double freq_hz = ghz * 1e9;
    double seconds = double(cycles) / freq_hz;
    return seconds * 1e9;
}
