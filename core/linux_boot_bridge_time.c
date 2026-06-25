/* linux_boot_bridge_time.c
 * Native Linux host-runtime implementations of the TIME/CLOCK FFI symbols
 * declared as plain externs in the Elisa sources (no @link_name / @c_abi),
 * so the C symbol names match the extern names exactly.
 *
 * Elisa extern signatures implemented here:
 *   extern FencedRDTSC() -> u64
 *   extern posix_clock_gettime_ns() -> u64
 *   extern high_resolution_now_ns() -> i64
 *   extern common_sleep_for_ms(milliseconds: u64) -> void
 */
#define _GNU_SOURCE
#include <stdint.h>
#include <time.h>
#include <sys/time.h>

uint64_t FencedRDTSC(void) {
#if defined(__x86_64__) || defined(__i386__)
    unsigned int lo, hi;
    __asm__ volatile("lfence; rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | (uint64_t)lo;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
#endif
}

uint64_t posix_clock_gettime_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

int64_t high_resolution_now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000000000LL + (int64_t)ts.tv_nsec;
}

void common_sleep_for_ms(uint64_t milliseconds) {
    struct timespec req;
    req.tv_sec  = (time_t)(milliseconds / 1000ULL);
    req.tv_nsec = (long)((milliseconds % 1000ULL) * 1000000ULL);
    struct timespec rem;
    while (nanosleep(&req, &rem) != 0) {
        req = rem;
    }
}
