#include <errno.h>
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
    case EAGAIN:
        return 35;
    case ETIMEDOUT:
        return 60;
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
