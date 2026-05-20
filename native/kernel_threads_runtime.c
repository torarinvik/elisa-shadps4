#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

typedef void *(*elisa_thread_entry_fn)(void *);

typedef struct ElisaKernelThread {
    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    elisa_thread_entry_fn entry;
    void *arg;
    void *result;
    int finished;
    int joined;
    int detached;
} ElisaKernelThread;

typedef struct ElisaKernelMutex {
    pthread_mutex_t mutex;
    pthread_t owner;
    int locked;
    int recursive;
    int recursion;
} ElisaKernelMutex;

typedef struct ElisaKernelCond {
    pthread_cond_t cond;
} ElisaKernelCond;

typedef struct ElisaKernelRwlock {
    pthread_rwlock_t rwlock;
} ElisaKernelRwlock;

typedef struct ElisaKernelSem {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int value;
    int max_value;
} ElisaKernelSem;

typedef struct ElisaKernelEventFlag {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    uint64_t bits;
    int waiters;
    int canceled;
} ElisaKernelEventFlag;

static int elisa_kernel_make_deadline_from_usec(uint64_t usec, struct timespec *out) {
    if (out == NULL) {
        return 22;
    }
    if (clock_gettime(CLOCK_REALTIME, out) != 0) {
        return errno;
    }
    out->tv_sec += (time_t)(usec / 1000000ULL);
    long nsec = out->tv_nsec + (long)((usec % 1000000ULL) * 1000ULL);
    if (nsec >= 1000000000L) {
        out->tv_sec += 1;
        nsec -= 1000000000L;
    }
    out->tv_nsec = nsec;
    return 0;
}

static int elisa_kernel_thread_errno(int err) {
    switch (err) {
    case 0:
        return 0;
    case EPERM:
        return 1;
    case ESRCH:
        return 3;
    case EDEADLK:
        return 11;
    case EINVAL:
        return 22;
    case EBUSY:
        return 16;
    case EAGAIN:
        return 35;
    case ETIMEDOUT:
        return 60;
    case EOVERFLOW:
        return 84;
    default:
        return err;
    }
}

static void *elisa_kernel_thread_trampoline(void *raw) {
    ElisaKernelThread *state = (ElisaKernelThread *)raw;
    void *result = NULL;
    if (state->entry != NULL) {
        result = state->entry(state->arg);
    }

    pthread_mutex_lock(&state->mutex);
    state->result = result;
    state->finished = 1;
    pthread_cond_broadcast(&state->cond);
    const int detached = state->detached;
    pthread_mutex_unlock(&state->mutex);

    if (detached) {
        pthread_mutex_destroy(&state->mutex);
        pthread_cond_destroy(&state->cond);
        free(state);
    }
    return result;
}

void *elisa_kernel_thread_create(void *entry, void *arg, int *out_error) {
    ElisaKernelThread *state = (ElisaKernelThread *)calloc(1, sizeof(*state));
    if (state == NULL) {
        if (out_error != NULL) {
            *out_error = 35;
        }
        return NULL;
    }

    pthread_mutex_init(&state->mutex, NULL);
    pthread_cond_init(&state->cond, NULL);
    state->entry = (elisa_thread_entry_fn)entry;
    state->arg = arg;

    int err = pthread_create(&state->thread, NULL, elisa_kernel_thread_trampoline, state);
    if (err != 0) {
        pthread_mutex_destroy(&state->mutex);
        pthread_cond_destroy(&state->cond);
        free(state);
        if (out_error != NULL) {
            *out_error = elisa_kernel_thread_errno(err);
        }
        return NULL;
    }

    if (out_error != NULL) {
        *out_error = 0;
    }
    return state;
}

void *elisa_kernel_thread_native(void *handle) {
    ElisaKernelThread *state = (ElisaKernelThread *)handle;
    if (state == NULL) {
        return NULL;
    }
    return (void *)(uintptr_t)state->thread;
}

int elisa_kernel_thread_join(void *handle, void **retval) {
    ElisaKernelThread *state = (ElisaKernelThread *)handle;
    if (state == NULL) {
        return 22;
    }

    pthread_mutex_lock(&state->mutex);
    if (state->detached || state->joined) {
        pthread_mutex_unlock(&state->mutex);
        return 22;
    }
    state->joined = 1;
    pthread_mutex_unlock(&state->mutex);

    void *result = NULL;
    int err = pthread_join(state->thread, &result);
    if (err != 0) {
        return elisa_kernel_thread_errno(err);
    }
    if (retval != NULL) {
        *retval = result;
    }

    pthread_mutex_destroy(&state->mutex);
    pthread_cond_destroy(&state->cond);
    free(state);
    return 0;
}

int elisa_kernel_thread_timedjoin(void *handle, void **retval, int64_t tv_sec, int64_t tv_nsec) {
    ElisaKernelThread *state = (ElisaKernelThread *)handle;
    if (state == NULL || tv_sec < 0 || tv_nsec < 0 || tv_nsec >= 1000000000LL) {
        return 22;
    }

    struct timespec deadline;
    deadline.tv_sec = (time_t)tv_sec;
    deadline.tv_nsec = (long)tv_nsec;

    pthread_mutex_lock(&state->mutex);
    if (state->detached || state->joined) {
        pthread_mutex_unlock(&state->mutex);
        return 22;
    }
    while (!state->finished) {
        int wait_err = pthread_cond_timedwait(&state->cond, &state->mutex, &deadline);
        if (wait_err == ETIMEDOUT) {
            pthread_mutex_unlock(&state->mutex);
            return 60;
        }
        if (wait_err != 0) {
            pthread_mutex_unlock(&state->mutex);
            return elisa_kernel_thread_errno(wait_err);
        }
    }
    state->joined = 1;
    pthread_mutex_unlock(&state->mutex);

    void *result = NULL;
    int err = pthread_join(state->thread, &result);
    if (err != 0) {
        return elisa_kernel_thread_errno(err);
    }
    if (retval != NULL) {
        *retval = result;
    }

    pthread_mutex_destroy(&state->mutex);
    pthread_cond_destroy(&state->cond);
    free(state);
    return 0;
}

int elisa_kernel_thread_detach(void *handle) {
    ElisaKernelThread *state = (ElisaKernelThread *)handle;
    if (state == NULL) {
        return 22;
    }

    pthread_mutex_lock(&state->mutex);
    if (state->detached || state->joined) {
        pthread_mutex_unlock(&state->mutex);
        return 22;
    }
    state->detached = 1;
    const int finished = state->finished;
    pthread_mutex_unlock(&state->mutex);

    int err = pthread_detach(state->thread);
    if (err != 0) {
        return elisa_kernel_thread_errno(err);
    }

    if (finished) {
        pthread_mutex_destroy(&state->mutex);
        pthread_cond_destroy(&state->cond);
        free(state);
    }
    return 0;
}

int elisa_kernel_thread_cancel(void *handle) {
    ElisaKernelThread *state = (ElisaKernelThread *)handle;
    if (state == NULL) {
        return 22;
    }
    return elisa_kernel_thread_errno(pthread_cancel(state->thread));
}

void *elisa_kernel_mutex_create(int type, int *out_error) {
    ElisaKernelMutex *state = (ElisaKernelMutex *)calloc(1, sizeof(*state));
    if (state == NULL) {
        if (out_error != NULL) {
            *out_error = 35;
        }
        return NULL;
    }

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    if (type == 2 || type == 4) {
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        state->recursive = 1;
    } else if (type == 3) {
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    }

    int err = pthread_mutex_init(&state->mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    if (err != 0) {
        free(state);
        if (out_error != NULL) {
            *out_error = elisa_kernel_thread_errno(err);
        }
        return NULL;
    }
    if (out_error != NULL) {
        *out_error = 0;
    }
    return state;
}

int elisa_kernel_mutex_destroy(void *handle) {
    ElisaKernelMutex *state = (ElisaKernelMutex *)handle;
    if (state == NULL) {
        return 22;
    }
    if (state->locked) {
        return 16;
    }
    int err = pthread_mutex_destroy(&state->mutex);
    free(state);
    return elisa_kernel_thread_errno(err);
}

int elisa_kernel_mutex_lock(void *handle) {
    ElisaKernelMutex *state = (ElisaKernelMutex *)handle;
    if (state == NULL) {
        return 22;
    }
    pthread_t self = pthread_self();
    if (state->locked && pthread_equal(state->owner, self)) {
        if (state->recursive) {
            state->recursion += 1;
            return 0;
        }
        return 16;
    }
    int err = pthread_mutex_lock(&state->mutex);
    if (err != 0) {
        return elisa_kernel_thread_errno(err);
    }
    state->owner = self;
    state->locked = 1;
    state->recursion = 1;
    return 0;
}

int elisa_kernel_mutex_trylock(void *handle) {
    ElisaKernelMutex *state = (ElisaKernelMutex *)handle;
    if (state == NULL) {
        return 22;
    }
    pthread_t self = pthread_self();
    if (state->locked && pthread_equal(state->owner, self)) {
        if (state->recursive) {
            state->recursion += 1;
            return 0;
        }
        return 16;
    }
    int err = pthread_mutex_trylock(&state->mutex);
    if (err != 0) {
        return elisa_kernel_thread_errno(err);
    }
    state->owner = self;
    state->locked = 1;
    state->recursion = 1;
    return 0;
}

int elisa_kernel_mutex_unlock(void *handle) {
    ElisaKernelMutex *state = (ElisaKernelMutex *)handle;
    if (state == NULL) {
        return 22;
    }
    if (!state->locked || !pthread_equal(state->owner, pthread_self())) {
        return 1;
    }
    if (state->recursive && state->recursion > 1) {
        state->recursion -= 1;
        return 0;
    }
    state->locked = 0;
    state->recursion = 0;
    int err = pthread_mutex_unlock(&state->mutex);
    return elisa_kernel_thread_errno(err);
}

int elisa_kernel_mutex_isowned(void *handle) {
    ElisaKernelMutex *state = (ElisaKernelMutex *)handle;
    if (state == NULL) {
        return 22;
    }
    return state->locked && pthread_equal(state->owner, pthread_self());
}

void *elisa_kernel_cond_create(int *out_error) {
    ElisaKernelCond *state = (ElisaKernelCond *)calloc(1, sizeof(*state));
    if (state == NULL) {
        if (out_error != NULL) {
            *out_error = 35;
        }
        return NULL;
    }
    int err = pthread_cond_init(&state->cond, NULL);
    if (err != 0) {
        free(state);
        if (out_error != NULL) {
            *out_error = elisa_kernel_thread_errno(err);
        }
        return NULL;
    }
    if (out_error != NULL) {
        *out_error = 0;
    }
    return state;
}

int elisa_kernel_cond_destroy(void *handle) {
    ElisaKernelCond *state = (ElisaKernelCond *)handle;
    if (state == NULL) {
        return 22;
    }
    int err = pthread_cond_destroy(&state->cond);
    free(state);
    return elisa_kernel_thread_errno(err);
}

int elisa_kernel_cond_wait(void *cond_handle, void *mutex_handle) {
    ElisaKernelCond *cond = (ElisaKernelCond *)cond_handle;
    ElisaKernelMutex *mutex = (ElisaKernelMutex *)mutex_handle;
    if (cond == NULL || mutex == NULL) {
        return 22;
    }
    if (!mutex->locked || !pthread_equal(mutex->owner, pthread_self())) {
        return 1;
    }
    mutex->locked = 0;
    mutex->recursion = 0;
    int err = pthread_cond_wait(&cond->cond, &mutex->mutex);
    if (err != 0) {
        return elisa_kernel_thread_errno(err);
    }
    mutex->owner = pthread_self();
    mutex->locked = 1;
    mutex->recursion = 1;
    return 0;
}

int elisa_kernel_cond_timedwait(void *cond_handle, void *mutex_handle, int64_t tv_sec, int64_t tv_nsec) {
    ElisaKernelCond *cond = (ElisaKernelCond *)cond_handle;
    ElisaKernelMutex *mutex = (ElisaKernelMutex *)mutex_handle;
    if (cond == NULL || mutex == NULL || tv_sec < 0 || tv_nsec < 0 || tv_nsec >= 1000000000LL) {
        return 22;
    }
    if (!mutex->locked || !pthread_equal(mutex->owner, pthread_self())) {
        return 1;
    }
    struct timespec deadline;
    deadline.tv_sec = (time_t)tv_sec;
    deadline.tv_nsec = (long)tv_nsec;
    mutex->locked = 0;
    mutex->recursion = 0;
    int err = pthread_cond_timedwait(&cond->cond, &mutex->mutex, &deadline);
    mutex->owner = pthread_self();
    mutex->locked = 1;
    mutex->recursion = 1;
    return elisa_kernel_thread_errno(err);
}

int elisa_kernel_cond_reltimedwait(void *cond_handle, void *mutex_handle, uint64_t usec) {
    struct timespec deadline;
    int err = elisa_kernel_make_deadline_from_usec(usec, &deadline);
    if (err != 0) {
        return elisa_kernel_thread_errno(err);
    }
    return elisa_kernel_cond_timedwait(cond_handle, mutex_handle, deadline.tv_sec, deadline.tv_nsec);
}

int elisa_kernel_cond_signal(void *handle) {
    ElisaKernelCond *state = (ElisaKernelCond *)handle;
    if (state == NULL) {
        return 22;
    }
    return elisa_kernel_thread_errno(pthread_cond_signal(&state->cond));
}

int elisa_kernel_cond_broadcast(void *handle) {
    ElisaKernelCond *state = (ElisaKernelCond *)handle;
    if (state == NULL) {
        return 22;
    }
    return elisa_kernel_thread_errno(pthread_cond_broadcast(&state->cond));
}

void *elisa_kernel_rwlock_create(int *out_error) {
    ElisaKernelRwlock *state = (ElisaKernelRwlock *)calloc(1, sizeof(*state));
    if (state == NULL) {
        if (out_error != NULL) {
            *out_error = 35;
        }
        return NULL;
    }
    int err = pthread_rwlock_init(&state->rwlock, NULL);
    if (err != 0) {
        free(state);
        if (out_error != NULL) {
            *out_error = elisa_kernel_thread_errno(err);
        }
        return NULL;
    }
    if (out_error != NULL) {
        *out_error = 0;
    }
    return state;
}

int elisa_kernel_rwlock_destroy(void *handle) {
    ElisaKernelRwlock *state = (ElisaKernelRwlock *)handle;
    if (state == NULL) {
        return 22;
    }
    int err = pthread_rwlock_destroy(&state->rwlock);
    free(state);
    return elisa_kernel_thread_errno(err);
}

int elisa_kernel_rwlock_rdlock(void *handle) {
    ElisaKernelRwlock *state = (ElisaKernelRwlock *)handle;
    if (state == NULL) {
        return 22;
    }
    return elisa_kernel_thread_errno(pthread_rwlock_rdlock(&state->rwlock));
}

int elisa_kernel_rwlock_tryrdlock(void *handle) {
    ElisaKernelRwlock *state = (ElisaKernelRwlock *)handle;
    if (state == NULL) {
        return 22;
    }
    return elisa_kernel_thread_errno(pthread_rwlock_tryrdlock(&state->rwlock));
}

int elisa_kernel_rwlock_wrlock(void *handle) {
    ElisaKernelRwlock *state = (ElisaKernelRwlock *)handle;
    if (state == NULL) {
        return 22;
    }
    return elisa_kernel_thread_errno(pthread_rwlock_wrlock(&state->rwlock));
}

int elisa_kernel_rwlock_trywrlock(void *handle) {
    ElisaKernelRwlock *state = (ElisaKernelRwlock *)handle;
    if (state == NULL) {
        return 22;
    }
    return elisa_kernel_thread_errno(pthread_rwlock_trywrlock(&state->rwlock));
}

int elisa_kernel_rwlock_unlock(void *handle) {
    ElisaKernelRwlock *state = (ElisaKernelRwlock *)handle;
    if (state == NULL) {
        return 22;
    }
    return elisa_kernel_thread_errno(pthread_rwlock_unlock(&state->rwlock));
}

void *elisa_kernel_sem_create(int value, int max_value, int *out_error) {
    ElisaKernelSem *state = (ElisaKernelSem *)calloc(1, sizeof(*state));
    if (state == NULL) {
        if (out_error != NULL) {
            *out_error = 35;
        }
        return NULL;
    }
    pthread_mutex_init(&state->mutex, NULL);
    pthread_cond_init(&state->cond, NULL);
    state->value = value;
    state->max_value = max_value;
    if (out_error != NULL) {
        *out_error = 0;
    }
    return state;
}

int elisa_kernel_sem_destroy(void *handle) {
    ElisaKernelSem *state = (ElisaKernelSem *)handle;
    if (state == NULL) {
        return 22;
    }
    pthread_mutex_destroy(&state->mutex);
    pthread_cond_destroy(&state->cond);
    free(state);
    return 0;
}

int elisa_kernel_sem_wait(void *handle, int need_count) {
    ElisaKernelSem *state = (ElisaKernelSem *)handle;
    if (state == NULL || need_count < 0) {
        return 22;
    }
    pthread_mutex_lock(&state->mutex);
    while (state->value < need_count) {
        pthread_cond_wait(&state->cond, &state->mutex);
    }
    state->value -= need_count;
    pthread_mutex_unlock(&state->mutex);
    return 0;
}

int elisa_kernel_sem_trywait(void *handle, int need_count) {
    ElisaKernelSem *state = (ElisaKernelSem *)handle;
    if (state == NULL || need_count < 0) {
        return 22;
    }
    pthread_mutex_lock(&state->mutex);
    if (state->value < need_count) {
        pthread_mutex_unlock(&state->mutex);
        return 35;
    }
    state->value -= need_count;
    pthread_mutex_unlock(&state->mutex);
    return 0;
}

int elisa_kernel_sem_timedwait(void *handle, int need_count, uint64_t usec) {
    ElisaKernelSem *state = (ElisaKernelSem *)handle;
    if (state == NULL || need_count < 0) {
        return 22;
    }
    struct timespec deadline;
    int err = elisa_kernel_make_deadline_from_usec(usec, &deadline);
    if (err != 0) {
        return elisa_kernel_thread_errno(err);
    }
    pthread_mutex_lock(&state->mutex);
    while (state->value < need_count) {
        err = pthread_cond_timedwait(&state->cond, &state->mutex, &deadline);
        if (err == ETIMEDOUT) {
            pthread_mutex_unlock(&state->mutex);
            return 60;
        }
        if (err != 0) {
            pthread_mutex_unlock(&state->mutex);
            return elisa_kernel_thread_errno(err);
        }
    }
    state->value -= need_count;
    pthread_mutex_unlock(&state->mutex);
    return 0;
}

int elisa_kernel_sem_post(void *handle, int signal_count) {
    ElisaKernelSem *state = (ElisaKernelSem *)handle;
    if (state == NULL || signal_count < 0) {
        return 22;
    }
    pthread_mutex_lock(&state->mutex);
    if (signal_count > state->max_value - state->value) {
        pthread_mutex_unlock(&state->mutex);
        return 84;
    }
    state->value += signal_count;
    pthread_cond_broadcast(&state->cond);
    pthread_mutex_unlock(&state->mutex);
    return 0;
}

int elisa_kernel_sem_getvalue(void *handle, int *out_value) {
    ElisaKernelSem *state = (ElisaKernelSem *)handle;
    if (state == NULL) {
        return 22;
    }
    pthread_mutex_lock(&state->mutex);
    if (out_value != NULL) {
        *out_value = state->value;
    }
    pthread_mutex_unlock(&state->mutex);
    return 0;
}

int elisa_kernel_sem_set(void *handle, int value) {
    ElisaKernelSem *state = (ElisaKernelSem *)handle;
    if (state == NULL || value < 0 || value > state->max_value) {
        return 22;
    }
    pthread_mutex_lock(&state->mutex);
    state->value = value;
    pthread_cond_broadcast(&state->cond);
    pthread_mutex_unlock(&state->mutex);
    return 0;
}

static int elisa_kernel_event_matches(uint64_t bits, uint64_t pattern, uint32_t wait_mode) {
    const uint32_t low = wait_mode & 0x0fU;
    if (low == 0x01U) {
        return (bits & pattern) == pattern;
    }
    if (low == 0x02U) {
        return (bits & pattern) != 0;
    }
    return 0;
}

static void elisa_kernel_event_apply_clear(ElisaKernelEventFlag *state, uint64_t pattern, uint32_t wait_mode) {
    const uint32_t clear_mode = wait_mode & 0xf0U;
    if (clear_mode == 0x10U) {
        state->bits = 0;
    } else if (clear_mode == 0x20U) {
        state->bits &= ~pattern;
    }
}

void *elisa_kernel_event_create(uint64_t init_pattern, int *out_error) {
    ElisaKernelEventFlag *state = (ElisaKernelEventFlag *)calloc(1, sizeof(*state));
    if (state == NULL) {
        if (out_error != NULL) {
            *out_error = 35;
        }
        return NULL;
    }
    pthread_mutex_init(&state->mutex, NULL);
    pthread_cond_init(&state->cond, NULL);
    state->bits = init_pattern;
    if (out_error != NULL) {
        *out_error = 0;
    }
    return state;
}

int elisa_kernel_event_destroy(void *handle) {
    ElisaKernelEventFlag *state = (ElisaKernelEventFlag *)handle;
    if (state == NULL) {
        return 22;
    }
    pthread_mutex_destroy(&state->mutex);
    pthread_cond_destroy(&state->cond);
    free(state);
    return 0;
}

int elisa_kernel_event_clear(void *handle, uint64_t mask) {
    ElisaKernelEventFlag *state = (ElisaKernelEventFlag *)handle;
    if (state == NULL) {
        return 22;
    }
    pthread_mutex_lock(&state->mutex);
    state->bits &= mask;
    pthread_mutex_unlock(&state->mutex);
    return 0;
}

int elisa_kernel_event_set(void *handle, uint64_t pattern) {
    ElisaKernelEventFlag *state = (ElisaKernelEventFlag *)handle;
    if (state == NULL) {
        return 22;
    }
    pthread_mutex_lock(&state->mutex);
    state->bits |= pattern;
    pthread_cond_broadcast(&state->cond);
    pthread_mutex_unlock(&state->mutex);
    return 0;
}

int elisa_kernel_event_cancel(void *handle, uint64_t set_pattern, int *out_waiters) {
    ElisaKernelEventFlag *state = (ElisaKernelEventFlag *)handle;
    if (state == NULL) {
        return 22;
    }
    pthread_mutex_lock(&state->mutex);
    const int waiters = state->waiters;
    state->bits = set_pattern;
    state->canceled = 1;
    if (out_waiters != NULL) {
        *out_waiters = waiters;
    }
    pthread_cond_broadcast(&state->cond);
    pthread_mutex_unlock(&state->mutex);
    return 0;
}

int elisa_kernel_event_poll(void *handle, uint64_t pattern, uint32_t wait_mode, uint64_t *out_bits) {
    ElisaKernelEventFlag *state = (ElisaKernelEventFlag *)handle;
    if (state == NULL || pattern == 0) {
        return 22;
    }
    pthread_mutex_lock(&state->mutex);
    const uint64_t bits = state->bits;
    if (out_bits != NULL) {
        *out_bits = bits;
    }
    if (!elisa_kernel_event_matches(bits, pattern, wait_mode)) {
        pthread_mutex_unlock(&state->mutex);
        return 35;
    }
    elisa_kernel_event_apply_clear(state, pattern, wait_mode);
    pthread_mutex_unlock(&state->mutex);
    return 0;
}

int elisa_kernel_event_wait(void *handle, uint64_t pattern, uint32_t wait_mode, uint64_t *out_bits, uint64_t usec) {
    ElisaKernelEventFlag *state = (ElisaKernelEventFlag *)handle;
    if (state == NULL || pattern == 0) {
        return 22;
    }
    struct timespec deadline;
    int err = 0;
    if (usec != UINT64_MAX) {
        err = elisa_kernel_make_deadline_from_usec(usec, &deadline);
        if (err != 0) {
            return elisa_kernel_thread_errno(err);
        }
    }

    pthread_mutex_lock(&state->mutex);
    state->waiters += 1;
    while (!elisa_kernel_event_matches(state->bits, pattern, wait_mode) && !state->canceled) {
        if (usec == UINT64_MAX) {
            err = pthread_cond_wait(&state->cond, &state->mutex);
        } else {
            err = pthread_cond_timedwait(&state->cond, &state->mutex, &deadline);
        }
        if (err == ETIMEDOUT) {
            state->waiters -= 1;
            pthread_mutex_unlock(&state->mutex);
            return 60;
        }
        if (err != 0) {
            state->waiters -= 1;
            pthread_mutex_unlock(&state->mutex);
            return elisa_kernel_thread_errno(err);
        }
    }
    state->waiters -= 1;
    const uint64_t bits = state->bits;
    if (out_bits != NULL) {
        *out_bits = bits;
    }
    if (state->canceled) {
        state->canceled = 0;
        pthread_mutex_unlock(&state->mutex);
        return 85;
    }
    elisa_kernel_event_apply_clear(state, pattern, wait_mode);
    pthread_mutex_unlock(&state->mutex);
    return 0;
}
