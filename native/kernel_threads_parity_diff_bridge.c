#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Elisa runtime-backed API (provided by core/libraries/kernel/threads/kt_runtime.elisa)
extern void *elisa_kernel_thread_create(void *entry, void *arg, int *out_error);
extern int elisa_kernel_thread_join(void *handle, void **retval);
extern int elisa_kernel_thread_detach(void *handle);
extern int elisa_kernel_thread_timedjoin(void *handle, void **retval, int64_t tv_sec, int64_t tv_nsec);
extern int elisa_kernel_thread_cancel(void *handle);
extern void *elisa_kernel_mutex_create(int type, int *out_error);
extern int elisa_kernel_mutex_destroy(void *handle);
extern int elisa_kernel_mutex_lock(void *handle);
extern int elisa_kernel_mutex_trylock(void *handle);
extern int elisa_kernel_mutex_unlock(void *handle);
extern int elisa_kernel_mutex_isowned(void *handle);
extern void *elisa_kernel_cond_create(int *out_error);
extern int elisa_kernel_cond_destroy(void *handle);
extern int elisa_kernel_cond_wait(void *cond_handle, void *mutex_handle);
extern int elisa_kernel_cond_timedwait(void *cond_handle, void *mutex_handle, int64_t tv_sec, int64_t tv_nsec);
extern int elisa_kernel_cond_signal(void *handle);
extern int elisa_kernel_cond_broadcast(void *handle);
extern void *elisa_kernel_rwlock_create(int *out_error);
extern int elisa_kernel_rwlock_destroy(void *handle);
extern int elisa_kernel_rwlock_rdlock(void *handle);
extern int elisa_kernel_rwlock_tryrdlock(void *handle);
extern int elisa_kernel_rwlock_wrlock(void *handle);
extern int elisa_kernel_rwlock_trywrlock(void *handle);
extern int elisa_kernel_rwlock_unlock(void *handle);
extern void *elisa_kernel_sem_create(int value, int max_value, int *out_error);
extern int elisa_kernel_sem_destroy(void *handle);
extern int elisa_kernel_sem_wait(void *handle, int need_count);
extern int elisa_kernel_sem_trywait(void *handle, int need_count);
extern int elisa_kernel_sem_timedwait(void *handle, int need_count, uint64_t usec);
extern int elisa_kernel_sem_post(void *handle, int signal_count);
extern int elisa_kernel_sem_getvalue(void *handle, int *out_value);
extern int elisa_kernel_sem_cancel(void *handle, int set_value, int *out_waiters);
extern int elisa_kernel_sem_delete(void *handle, int *out_waiters);
extern void *elisa_kernel_event_create(uint64_t init_pattern, int *out_error);
extern int elisa_kernel_event_destroy(void *handle);
extern int elisa_kernel_event_delete(void *handle, int *out_waiters);
extern int elisa_kernel_event_set(void *handle, uint64_t pattern);
extern int elisa_kernel_event_cancel(void *handle, uint64_t set_pattern, int *out_waiters);
extern int elisa_kernel_event_poll(void *handle, uint64_t pattern, uint32_t wait_mode, uint64_t *out_bits);
extern int elisa_kernel_event_wait(void *handle, uint64_t pattern, uint32_t wait_mode, uint64_t *out_bits, uint64_t usec);

#define POSIX_EINVAL_ELISA 22
#define POSIX_ETIMEDOUT_ELISA 60

int parity_host_is_windows(void) {
#ifdef _WIN32
    return 1;
#else
    return 0;
#endif
}

typedef void *(*entry_fn_t)(void *);

typedef struct OracleThread {
    pthread_t thread;
    int detached;
    int joined;
} OracleThread;

static pthread_key_t g_parity_tls_key;
static pthread_once_t g_parity_tls_once = PTHREAD_ONCE_INIT;
static volatile int g_oracle_cancel_started = 0;
static volatile int g_oracle_cancel_cleanup = 0;
static volatile int g_oracle_cancel_tls_dtor = 0;
static volatile int g_elisa_cancel_started = 0;
static volatile int g_elisa_cancel_cleanup = 0;
static volatile int g_elisa_cancel_tls_dtor = 0;

static void parity_tls_dtor(void *value) {
    if (value != NULL) {
        volatile int *counter = (volatile int *)value;
        __sync_add_and_fetch(counter, 1);
    }
}

static void parity_tls_key_init(void) {
    (void)pthread_key_create(&g_parity_tls_key, parity_tls_dtor);
}

static void parity_cancel_cleanup(void *raw) {
    volatile int *counter = (volatile int *)raw;
    __sync_add_and_fetch(counter, 1);
}

static void *oracle_worker_fn(void *arg) {
    uintptr_t v = (uintptr_t)arg;
    if (v == 1u) {
        struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = 50000000L;
        nanosleep(&ts, NULL);
        return (void *)0x77;
    }
    if (v == 2u) {
        return (void *)0x123456;
    }
    if (v == 3u || v == 4u) {
        volatile int *started = (v == 3u) ? &g_oracle_cancel_started : &g_elisa_cancel_started;
        volatile int *cleanup = (v == 3u) ? &g_oracle_cancel_cleanup : &g_elisa_cancel_cleanup;
        volatile int *tls_dtor = (v == 3u) ? &g_oracle_cancel_tls_dtor : &g_elisa_cancel_tls_dtor;
        pthread_once(&g_parity_tls_once, parity_tls_key_init);
        pthread_cleanup_push(parity_cancel_cleanup, (void *)cleanup);
        (void)pthread_setspecific(g_parity_tls_key, (void *)tls_dtor);
        __sync_lock_test_and_set(started, 1);
        for (;;) {
            struct timespec ts;
            ts.tv_sec = 0;
            ts.tv_nsec = 5000000L;
            nanosleep(&ts, NULL);
        }
        pthread_cleanup_pop(0);
    }
    return NULL;
}

void *oracle_thread_create(int mode, int *out_error) {
    OracleThread *ot = (OracleThread *)calloc(1, sizeof(*ot));
    if (!ot) {
        if (out_error) *out_error = 35;
        return NULL;
    }
    int err = pthread_create(&ot->thread, NULL, oracle_worker_fn, (void *)(uintptr_t)mode);
    if (err != 0) {
        free(ot);
        if (out_error) *out_error = err;
        return NULL;
    }
    if (out_error) *out_error = 0;
    return ot;
}

int oracle_thread_join(void *handle, void **retval) {
    OracleThread *ot = (OracleThread *)handle;
    if (!ot) return POSIX_EINVAL_ELISA;
    if (ot->detached || ot->joined) return POSIX_EINVAL_ELISA;
    ot->joined = 1;
    void *ret = NULL;
    int err = pthread_join(ot->thread, &ret);
    if (err != 0) return err;
    if (retval) *retval = ret;
    free(ot);
    return 0;
}

int oracle_thread_detach(void *handle) {
    OracleThread *ot = (OracleThread *)handle;
    if (!ot) return POSIX_EINVAL_ELISA;
    if (ot->detached || ot->joined) return POSIX_EINVAL_ELISA;
    int err = pthread_detach(ot->thread);
    if (err != 0) return err;
    ot->detached = 1;
    free(ot);
    return 0;
}

int oracle_thread_timedjoin_like_cpp(void *handle, void **retval) {
    return oracle_thread_join(handle, retval);
}

int oracle_thread_cancel(void *handle) {
    OracleThread *ot = (OracleThread *)handle;
    if (!ot) return POSIX_EINVAL_ELISA;
    return pthread_cancel(ot->thread);
}

int oracle_thread_getname_self(char *buf, uint64_t len) {
    if (!buf || len == 0) return POSIX_EINVAL_ELISA;
#if defined(__APPLE__) || defined(__linux__)
    int err = pthread_getname_np(pthread_self(), buf, (size_t)len);
    if (err != 0) return POSIX_EINVAL_ELISA;
    return 0;
#else
    (void)buf;
    (void)len;
    return POSIX_EINVAL_ELISA;
#endif
}

int oracle_thread_setname_self(const char *name) {
#if defined(__APPLE__) || defined(__linux__)
    if (!name) return 0;
    int err = pthread_setname_np(name);
    if (err != 0) return POSIX_EINVAL_ELISA;
    return 0;
#else
    (void)name;
    return 0;
#endif
}

int oracle_thread_getaffinity_self(uint64_t *mask) {
    if (!mask) return POSIX_EINVAL_ELISA;
    *mask = 0;
    return 0;
}

int oracle_thread_setaffinity_self(uint64_t mask) {
    (void)mask;
    return 0;
}

void *oracle_mutex_create(int type, int *out_error) {
    return elisa_kernel_mutex_create(type, out_error);
}
int oracle_mutex_destroy(void *h) { return elisa_kernel_mutex_destroy(h); }
int oracle_mutex_lock(void *h) { return elisa_kernel_mutex_lock(h); }
int oracle_mutex_trylock(void *h) { return elisa_kernel_mutex_trylock(h); }
int oracle_mutex_unlock(void *h) { return elisa_kernel_mutex_unlock(h); }
int oracle_mutex_isowned(void *h) { return elisa_kernel_mutex_isowned(h); }

void *oracle_cond_create(int *out_error) { return elisa_kernel_cond_create(out_error); }
int oracle_cond_destroy(void *h) { return elisa_kernel_cond_destroy(h); }
int oracle_cond_wait(void *c, void *m) { return elisa_kernel_cond_wait(c, m); }
int oracle_cond_timedwait(void *c, void *m, int64_t s, int64_t n) { return elisa_kernel_cond_timedwait(c, m, s, n); }
int oracle_cond_signal(void *h) { return elisa_kernel_cond_signal(h); }
int oracle_cond_broadcast(void *h) { return elisa_kernel_cond_broadcast(h); }

void *oracle_rwlock_create(int *out_error) { return elisa_kernel_rwlock_create(out_error); }
int oracle_rwlock_destroy(void *h) { return elisa_kernel_rwlock_destroy(h); }
int oracle_rwlock_rdlock(void *h) { return elisa_kernel_rwlock_rdlock(h); }
int oracle_rwlock_tryrdlock(void *h) { return elisa_kernel_rwlock_tryrdlock(h); }
int oracle_rwlock_wrlock(void *h) { return elisa_kernel_rwlock_wrlock(h); }
int oracle_rwlock_trywrlock(void *h) { return elisa_kernel_rwlock_trywrlock(h); }
int oracle_rwlock_unlock(void *h) { return elisa_kernel_rwlock_unlock(h); }

void *oracle_sem_create(int v, int maxv, int *out_error) { return elisa_kernel_sem_create(v, maxv, out_error); }
int oracle_sem_destroy(void *h) { return elisa_kernel_sem_destroy(h); }
int oracle_sem_wait(void *h, int n) { return elisa_kernel_sem_wait(h, n); }
int oracle_sem_trywait(void *h, int n) { return elisa_kernel_sem_trywait(h, n); }
int oracle_sem_timedwait(void *h, int n, uint64_t u) { return elisa_kernel_sem_timedwait(h, n, u); }
int oracle_sem_post(void *h, int n) { return elisa_kernel_sem_post(h, n); }
int oracle_sem_getvalue(void *h, int *v) { return elisa_kernel_sem_getvalue(h, v); }
int oracle_sem_cancel(void *h, int v, int *w) { return elisa_kernel_sem_cancel(h, v, w); }
int oracle_sem_delete(void *h, int *w) { return elisa_kernel_sem_delete(h, w); }

void *oracle_event_create(uint64_t p, int *out_error) { return elisa_kernel_event_create(p, out_error); }
int oracle_event_destroy(void *h) { return elisa_kernel_event_destroy(h); }
int oracle_event_delete(void *h, int *w) { return elisa_kernel_event_delete(h, w); }
int oracle_event_set(void *h, uint64_t p) { return elisa_kernel_event_set(h, p); }
int oracle_event_cancel(void *h, uint64_t p, int *w) { return elisa_kernel_event_cancel(h, p, w); }
int oracle_event_poll(void *h, uint64_t p, uint32_t m, uint64_t *o) { return elisa_kernel_event_poll(h, p, m, o); }
int oracle_event_wait(void *h, uint64_t p, uint32_t m, uint64_t *o, uint64_t u) { return elisa_kernel_event_wait(h, p, m, o, u); }

// Elisa adapter mirrors the same normalized ABI.
void *elisa_adapter_thread_create(int mode, int *out_error) {
    return elisa_kernel_thread_create((void *)oracle_worker_fn, (void *)(uintptr_t)mode, out_error);
}
int elisa_adapter_thread_join(void *h, void **r) { return elisa_kernel_thread_join(h, r); }
int elisa_adapter_thread_detach(void *h) { return elisa_kernel_thread_detach(h); }
int elisa_adapter_thread_timedjoin_like_cpp(void *h, void **r) { return elisa_kernel_thread_join(h, r); }
int elisa_adapter_thread_cancel(void *h) { return elisa_kernel_thread_cancel(h); }

int elisa_adapter_thread_getname_self(char *buf, uint64_t len) {
    if (!buf || len == 0) return POSIX_EINVAL_ELISA;
#if defined(__APPLE__) || defined(__linux__)
    int err = pthread_getname_np(pthread_self(), buf, (size_t)len);
    if (err != 0) return POSIX_EINVAL_ELISA;
#endif
    return 0;
}

int elisa_adapter_thread_setname_self(const char *name) {
#if defined(__APPLE__) || defined(__linux__)
    if (!name) return 0;
    int err = pthread_setname_np(name);
    if (err != 0) return POSIX_EINVAL_ELISA;
#endif
    return 0;
}

int elisa_adapter_thread_getaffinity_self(uint64_t *mask) {
    if (!mask) return POSIX_EINVAL_ELISA;
    *mask = 0;
    return 0;
}

int elisa_adapter_thread_setaffinity_self(uint64_t mask) {
    (void)mask;
    return 0;
}

void *elisa_adapter_mutex_create(int type, int *out_error) { return elisa_kernel_mutex_create(type, out_error); }
int elisa_adapter_mutex_destroy(void *h) { return elisa_kernel_mutex_destroy(h); }
int elisa_adapter_mutex_lock(void *h) { return elisa_kernel_mutex_lock(h); }
int elisa_adapter_mutex_trylock(void *h) { return elisa_kernel_mutex_trylock(h); }
int elisa_adapter_mutex_unlock(void *h) { return elisa_kernel_mutex_unlock(h); }
int elisa_adapter_mutex_isowned(void *h) { return elisa_kernel_mutex_isowned(h); }

void *elisa_adapter_cond_create(int *out_error) { return elisa_kernel_cond_create(out_error); }
int elisa_adapter_cond_destroy(void *h) { return elisa_kernel_cond_destroy(h); }
int elisa_adapter_cond_wait(void *c, void *m) { return elisa_kernel_cond_wait(c, m); }
int elisa_adapter_cond_timedwait(void *c, void *m, int64_t s, int64_t n) { return elisa_kernel_cond_timedwait(c, m, s, n); }
int elisa_adapter_cond_signal(void *h) { return elisa_kernel_cond_signal(h); }
int elisa_adapter_cond_broadcast(void *h) { return elisa_kernel_cond_broadcast(h); }

void *elisa_adapter_rwlock_create(int *out_error) { return elisa_kernel_rwlock_create(out_error); }
int elisa_adapter_rwlock_destroy(void *h) { return elisa_kernel_rwlock_destroy(h); }
int elisa_adapter_rwlock_rdlock(void *h) { return elisa_kernel_rwlock_rdlock(h); }
int elisa_adapter_rwlock_tryrdlock(void *h) { return elisa_kernel_rwlock_tryrdlock(h); }
int elisa_adapter_rwlock_wrlock(void *h) { return elisa_kernel_rwlock_wrlock(h); }
int elisa_adapter_rwlock_trywrlock(void *h) { return elisa_kernel_rwlock_trywrlock(h); }
int elisa_adapter_rwlock_unlock(void *h) { return elisa_kernel_rwlock_unlock(h); }

void *elisa_adapter_sem_create(int v, int maxv, int *out_error) { return elisa_kernel_sem_create(v, maxv, out_error); }
int elisa_adapter_sem_destroy(void *h) { return elisa_kernel_sem_destroy(h); }
int elisa_adapter_sem_wait(void *h, int n) { return elisa_kernel_sem_wait(h, n); }
int elisa_adapter_sem_trywait(void *h, int n) { return elisa_kernel_sem_trywait(h, n); }
int elisa_adapter_sem_timedwait(void *h, int n, uint64_t u) { return elisa_kernel_sem_timedwait(h, n, u); }
int elisa_adapter_sem_post(void *h, int n) { return elisa_kernel_sem_post(h, n); }
int elisa_adapter_sem_getvalue(void *h, int *v) { return elisa_kernel_sem_getvalue(h, v); }
int elisa_adapter_sem_cancel(void *h, int v, int *w) { return elisa_kernel_sem_cancel(h, v, w); }
int elisa_adapter_sem_delete(void *h, int *w) { return elisa_kernel_sem_delete(h, w); }

void *elisa_adapter_event_create(uint64_t p, int *out_error) { return elisa_kernel_event_create(p, out_error); }
int elisa_adapter_event_destroy(void *h) { return elisa_kernel_event_destroy(h); }
int elisa_adapter_event_delete(void *h, int *w) { return elisa_kernel_event_delete(h, w); }
int elisa_adapter_event_set(void *h, uint64_t p) { return elisa_kernel_event_set(h, p); }
int elisa_adapter_event_cancel(void *h, uint64_t p, int *w) { return elisa_kernel_event_cancel(h, p, w); }
int elisa_adapter_event_poll(void *h, uint64_t p, uint32_t m, uint64_t *o) { return elisa_kernel_event_poll(h, p, m, o); }
int elisa_adapter_event_wait(void *h, uint64_t p, uint32_t m, uint64_t *o, uint64_t u) { return elisa_kernel_event_wait(h, p, m, o, u); }

typedef struct ParityWaitArg {
    void *handle;
    int is_event;
    int result;
} ParityWaitArg;

static void *parity_wait_worker(void *raw) {
    ParityWaitArg *arg = (ParityWaitArg *)raw;
    if (arg->is_event) {
        uint64_t out = 0;
        arg->result = elisa_kernel_event_wait(arg->handle, 0x1u, 0x02u, &out, 250000u);
    } else {
        arg->result = elisa_kernel_sem_wait(arg->handle, 1);
    }
    return NULL;
}

int oracle_sem_delete_wake_scenario(void) {
    int err = 0;
    int waiters = 0;
    void *sem = oracle_sem_create(0, 4, &err);
    if (!sem || err != 0) {
        return -1;
    }
    ParityWaitArg wa;
    memset(&wa, 0, sizeof(wa));
    wa.handle = sem;
    wa.is_event = 0;
    pthread_t t;
    if (pthread_create(&t, NULL, parity_wait_worker, &wa) != 0) {
        oracle_sem_destroy(sem);
        return -2;
    }
    struct timespec ts = {.tv_sec = 0, .tv_nsec = 20000000L};
    nanosleep(&ts, NULL);
    int del_ret = oracle_sem_delete(sem, &waiters);
    pthread_join(t, NULL);
    return ((del_ret & 0xFFFF) << 16) | (wa.result & 0xFFFF);
}

int elisa_adapter_sem_delete_wake_scenario(void) {
    int err = 0;
    int waiters = 0;
    void *sem = elisa_adapter_sem_create(0, 4, &err);
    if (!sem || err != 0) {
        return -1;
    }
    ParityWaitArg wa;
    memset(&wa, 0, sizeof(wa));
    wa.handle = sem;
    wa.is_event = 0;
    pthread_t t;
    if (pthread_create(&t, NULL, parity_wait_worker, &wa) != 0) {
        elisa_adapter_sem_destroy(sem);
        return -2;
    }
    struct timespec ts = {.tv_sec = 0, .tv_nsec = 20000000L};
    nanosleep(&ts, NULL);
    int del_ret = elisa_adapter_sem_delete(sem, &waiters);
    pthread_join(t, NULL);
    return ((del_ret & 0xFFFF) << 16) | (wa.result & 0xFFFF);
}

int oracle_event_cancel_wake_scenario(void) {
    int err = 0;
    int waiters = 0;
    void *ev = oracle_event_create(0, &err);
    if (!ev || err != 0) {
        return -1;
    }
    ParityWaitArg wa;
    memset(&wa, 0, sizeof(wa));
    wa.handle = ev;
    wa.is_event = 1;
    pthread_t t;
    if (pthread_create(&t, NULL, parity_wait_worker, &wa) != 0) {
        oracle_event_destroy(ev);
        return -2;
    }
    struct timespec ts = {.tv_sec = 0, .tv_nsec = 20000000L};
    nanosleep(&ts, NULL);
    int cancel_ret = oracle_event_cancel(ev, 0, &waiters);
    pthread_join(t, NULL);
    oracle_event_destroy(ev);
    return ((cancel_ret & 0xFFFF) << 16) | (wa.result & 0xFFFF);
}

int elisa_adapter_event_cancel_wake_scenario(void) {
    int err = 0;
    int waiters = 0;
    void *ev = elisa_adapter_event_create(0, &err);
    if (!ev || err != 0) {
        return -1;
    }
    ParityWaitArg wa;
    memset(&wa, 0, sizeof(wa));
    wa.handle = ev;
    wa.is_event = 1;
    pthread_t t;
    if (pthread_create(&t, NULL, parity_wait_worker, &wa) != 0) {
        elisa_adapter_event_destroy(ev);
        return -2;
    }
    struct timespec ts = {.tv_sec = 0, .tv_nsec = 20000000L};
    nanosleep(&ts, NULL);
    int cancel_ret = elisa_adapter_event_cancel(ev, 0, &waiters);
    pthread_join(t, NULL);
    elisa_adapter_event_destroy(ev);
    return ((cancel_ret & 0xFFFF) << 16) | (wa.result & 0xFFFF);
}

static int parity_run_cancel_cleanup_tls_scenario(int use_elisa) {
    int err = 0;
    void *handle = NULL;
    void *ret = NULL;
    int cancel_ret = 0;
    int join_ret = 0;
    if (use_elisa) {
        g_elisa_cancel_started = 0;
        g_elisa_cancel_cleanup = 0;
        g_elisa_cancel_tls_dtor = 0;
        handle = elisa_adapter_thread_create(4, &err);
    } else {
        g_oracle_cancel_started = 0;
        g_oracle_cancel_cleanup = 0;
        g_oracle_cancel_tls_dtor = 0;
        handle = oracle_thread_create(3, &err);
    }
    if (handle == NULL || err != 0) {
        return -1;
    }

    for (int i = 0; i < 250; ++i) {
        int started = use_elisa ? g_elisa_cancel_started : g_oracle_cancel_started;
        if (started != 0) {
            break;
        }
        struct timespec ts = {.tv_sec = 0, .tv_nsec = 1000000L};
        nanosleep(&ts, NULL);
    }

    if (use_elisa ? (g_elisa_cancel_started == 0) : (g_oracle_cancel_started == 0)) {
        return -2;
    }

    cancel_ret = use_elisa ? elisa_adapter_thread_cancel(handle) : oracle_thread_cancel(handle);
    join_ret = use_elisa ? elisa_adapter_thread_join(handle, &ret) : oracle_thread_join(handle, &ret);
    if (cancel_ret != 0 || join_ret != 0) {
        return -3;
    }

    if (use_elisa) {
        if (g_elisa_cancel_cleanup <= 0 || g_elisa_cancel_tls_dtor <= 0) {
            return -4;
        }
    } else {
        if (g_oracle_cancel_cleanup <= 0 || g_oracle_cancel_tls_dtor <= 0) {
            return -4;
        }
    }

    if (ret != PTHREAD_CANCELED) {
        return -5;
    }
    return 0;
}

int oracle_cancel_cleanup_tls_scenario(void) {
    return parity_run_cancel_cleanup_tls_scenario(0);
}

int elisa_adapter_cancel_cleanup_tls_scenario(void) {
    return parity_run_cancel_cleanup_tls_scenario(1);
}
