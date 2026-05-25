#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#define ELISA_KERNEL_TEST_SLOT_COUNT 64

static pthread_mutex_t g_slots_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_slots_cond = PTHREAD_COND_INITIALIZER;
static int64_t g_slots[ELISA_KERNEL_TEST_SLOT_COUNT];

// Elisa runtime-backed thread API (provided by core/libraries/kernel/threads/kt_runtime.elisa).
extern void *elisa_kernel_thread_create(void *entry, void *arg, int *out_error);
extern int elisa_kernel_thread_join(void *handle, void **retval);
extern int elisa_kernel_thread_timedjoin(void *handle, void **retval, int64_t tv_sec, int64_t tv_nsec);
extern int elisa_kernel_thread_cancel(void *handle);

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

static pthread_key_t g_cancel_tls_key;
static pthread_once_t g_cancel_tls_once = PTHREAD_ONCE_INIT;

static void cancel_tls_destructor(void *value) {
    int slot = (int)(intptr_t)value;
    if (slot >= 0) {
        elisa_kernel_test_slot_add(slot, 1);
    }
}

static void cancel_tls_key_init(void) {
    (void)pthread_key_create(&g_cancel_tls_key, cancel_tls_destructor);
}

static void cancel_cleanup_handler(void *raw) {
    int slot = *(int *)raw;
    if (slot >= 0) {
        elisa_kernel_test_slot_add(slot, 1);
    }
}

typedef struct CancelThreadArgs {
    int started_slot;
    int cleanup_slot;
    int tls_slot;
} CancelThreadArgs;

static void *cancel_worker(void *raw) {
    CancelThreadArgs *args = (CancelThreadArgs *)raw;
    pthread_once(&g_cancel_tls_once, cancel_tls_key_init);
    pthread_cleanup_push(cancel_cleanup_handler, &args->cleanup_slot);
    (void)pthread_setspecific(g_cancel_tls_key, (void *)(intptr_t)args->tls_slot);
    elisa_kernel_test_slot_set(args->started_slot, 1);
    for (;;) {
        struct timespec ts = {.tv_sec = 0, .tv_nsec = 5000000L};
        (void)nanosleep(&ts, NULL);
    }
    pthread_cleanup_pop(0);
    return NULL;
}

int elisa_kernel_test_cancel_cleanup_tls_join_scenario(void) {
    CancelThreadArgs args = {.started_slot = 40, .cleanup_slot = 41, .tls_slot = 42};
    int create_err = 0;
    void *thr = elisa_kernel_thread_create((void *)cancel_worker, &args, &create_err);
    if (thr == NULL || create_err != 0) {
        return -1;
    }
    if (elisa_kernel_test_wait_slot(args.started_slot, 1, 250000ULL) != 0) {
        return -2;
    }
    int cancel_ret = elisa_kernel_thread_cancel(thr);
    void *ret = NULL;
    int join_ret = elisa_kernel_thread_join(thr, &ret);
    if (cancel_ret != 0 || join_ret != 0) {
        return -3;
    }
    if (elisa_kernel_test_wait_slot_at_least(args.cleanup_slot, 1, 250000ULL) != 0) {
        return -4;
    }
    if (elisa_kernel_test_wait_slot_at_least(args.tls_slot, 1, 250000ULL) != 0) {
        return -5;
    }
    if (ret != PTHREAD_CANCELED) {
        return -6;
    }
    return 0;
}

int elisa_kernel_test_cancel_cleanup_tls_timedjoin_scenario(void) {
    CancelThreadArgs args = {.started_slot = 43, .cleanup_slot = 44, .tls_slot = 45};
    int create_err = 0;
    void *thr = elisa_kernel_thread_create((void *)cancel_worker, &args, &create_err);
    if (thr == NULL || create_err != 0) {
        return -1;
    }
    if (elisa_kernel_test_wait_slot(args.started_slot, 1, 250000ULL) != 0) {
        return -2;
    }
    if (elisa_kernel_thread_cancel(thr) != 0) {
        return -3;
    }
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    int64_t sec = (int64_t)now.tv_sec;
    int64_t nsec = (int64_t)now.tv_nsec + 200000000LL;
    if (nsec >= 1000000000LL) {
        sec += 1;
        nsec -= 1000000000LL;
    }
    void *ret = NULL;
    int join_ret = elisa_kernel_thread_timedjoin(thr, &ret, sec, nsec);
    if (join_ret != 0) {
        return -4;
    }
    if (elisa_kernel_test_wait_slot_at_least(args.cleanup_slot, 1, 250000ULL) != 0) {
        return -5;
    }
    if (elisa_kernel_test_wait_slot_at_least(args.tls_slot, 1, 250000ULL) != 0) {
        return -6;
    }
    if (ret != PTHREAD_CANCELED) {
        return -7;
    }
    return 0;
}
