#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#define ELISA_KERNEL_TEST_SLOT_COUNT 64

static pthread_mutex_t g_slots_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_slots_cond = PTHREAD_COND_INITIALIZER;
static int64_t g_slots[ELISA_KERNEL_TEST_SLOT_COUNT];

static int valid_slot(int slot) {
    return slot >= 0 && slot < ELISA_KERNEL_TEST_SLOT_COUNT;
}

static void make_deadline(uint64_t usec, struct timespec *out) {
    clock_gettime(CLOCK_REALTIME, out);
    out->tv_sec += (time_t)(usec / 1000000ULL);
    long nsec = out->tv_nsec + (long)((usec % 1000000ULL) * 1000ULL);
    if (nsec >= 1000000000L) {
        out->tv_sec += 1;
        nsec -= 1000000000L;
    }
    out->tv_nsec = nsec;
}

void elisa_kernel_test_slots_reset(void) {
    pthread_mutex_lock(&g_slots_mutex);
    for (int i = 0; i < ELISA_KERNEL_TEST_SLOT_COUNT; ++i) {
        g_slots[i] = 0;
    }
    pthread_cond_broadcast(&g_slots_cond);
    pthread_mutex_unlock(&g_slots_mutex);
}

void elisa_kernel_test_slot_set(int slot, int64_t value) {
    if (!valid_slot(slot)) {
        return;
    }
    pthread_mutex_lock(&g_slots_mutex);
    g_slots[slot] = value;
    pthread_cond_broadcast(&g_slots_cond);
    pthread_mutex_unlock(&g_slots_mutex);
}

int64_t elisa_kernel_test_slot_get(int slot) {
    if (!valid_slot(slot)) {
        return 0;
    }
    pthread_mutex_lock(&g_slots_mutex);
    int64_t value = g_slots[slot];
    pthread_mutex_unlock(&g_slots_mutex);
    return value;
}

int64_t elisa_kernel_test_slot_add(int slot, int64_t delta) {
    if (!valid_slot(slot)) {
        return 0;
    }
    pthread_mutex_lock(&g_slots_mutex);
    g_slots[slot] += delta;
    int64_t value = g_slots[slot];
    pthread_cond_broadcast(&g_slots_cond);
    pthread_mutex_unlock(&g_slots_mutex);
    return value;
}

int elisa_kernel_test_wait_slot(int slot, int64_t expected, uint64_t timeout_usec) {
    if (!valid_slot(slot)) {
        return 22;
    }
    struct timespec deadline;
    make_deadline(timeout_usec, &deadline);

    pthread_mutex_lock(&g_slots_mutex);
    while (g_slots[slot] != expected) {
        int err = pthread_cond_timedwait(&g_slots_cond, &g_slots_mutex, &deadline);
        if (err == ETIMEDOUT) {
            pthread_mutex_unlock(&g_slots_mutex);
            return 60;
        }
        if (err != 0) {
            pthread_mutex_unlock(&g_slots_mutex);
            return err;
        }
    }
    pthread_mutex_unlock(&g_slots_mutex);
    return 0;
}

int elisa_kernel_test_wait_slot_at_least(int slot, int64_t expected, uint64_t timeout_usec) {
    if (!valid_slot(slot)) {
        return 22;
    }
    struct timespec deadline;
    make_deadline(timeout_usec, &deadline);

    pthread_mutex_lock(&g_slots_mutex);
    while (g_slots[slot] < expected) {
        int err = pthread_cond_timedwait(&g_slots_cond, &g_slots_mutex, &deadline);
        if (err == ETIMEDOUT) {
            pthread_mutex_unlock(&g_slots_mutex);
            return 60;
        }
        if (err != 0) {
            pthread_mutex_unlock(&g_slots_mutex);
            return err;
        }
    }
    pthread_mutex_unlock(&g_slots_mutex);
    return 0;
}

uint64_t elisa_kernel_test_monotonic_usec(void) {
    struct timespec now;
#ifdef CLOCK_MONOTONIC
    clock_gettime(CLOCK_MONOTONIC, &now);
#else
    clock_gettime(CLOCK_REALTIME, &now);
#endif
    return ((uint64_t)now.tv_sec * 1000000ULL) + ((uint64_t)now.tv_nsec / 1000ULL);
}

void elisa_kernel_test_sleep_usec(uint64_t usec) {
    struct timespec req;
    req.tv_sec = (time_t)(usec / 1000000ULL);
    req.tv_nsec = (long)((usec % 1000000ULL) * 1000ULL);
    while (nanosleep(&req, &req) != 0 && errno == EINTR) {
    }
}

void elisa_kernel_test_yield(void) {
    sched_yield();
}
