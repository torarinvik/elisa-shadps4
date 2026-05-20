// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <zlib.h>

#define ORBIS_OK 0
#define ORBIS_ZLIB_ERROR_NOT_FOUND ((int)0x81120002)
#define ORBIS_ZLIB_ERROR_INVALID ((int)0x81120016)
#define ORBIS_ZLIB_ERROR_NOSPACE ((int)0x8112001C)
#define ORBIS_ZLIB_ERROR_TIMEDOUT ((int)0x81120027)
#define ORBIS_ZLIB_ERROR_NOT_INITIALIZED ((int)0x81120032)
#define ORBIS_ZLIB_ERROR_ALREADY_INITIALIZED ((int)0x81120033)
#define ORBIS_ZLIB_ERROR_FATAL ((int)0x811200FF)

#define MAX_RESULTS 1024
#define MAX_QUEUE 1024
#define KB_2 2048u
#define KB_64 65536u

typedef struct {
    uint64_t request_id;
    const void* src;
    uint32_t src_length;
    void* dst;
    uint32_t dst_length;
} InflateTask;

typedef struct {
    uint32_t length;
    int32_t status;
} InflateResult;

typedef struct {
    uint64_t request_id;
    InflateResult result;
    bool used;
} ResultSlot;

static pthread_t g_thread;
static bool g_thread_started = false;
static bool g_stop_requested = false;

static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_task_cv = PTHREAD_COND_INITIALIZER;
static pthread_cond_t g_done_cv = PTHREAD_COND_INITIALIZER;

static InflateTask g_task_q[MAX_QUEUE];
static int g_task_head = 0;
static int g_task_count = 0;

static uint64_t g_done_q[MAX_QUEUE];
static int g_done_head = 0;
static int g_done_count = 0;

static ResultSlot g_results[MAX_RESULTS];
static uint64_t g_next_request_id = 1;
static uint64_t g_test_delay_usec = 0;

static void clear_state_locked(void) {
    g_task_head = 0;
    g_task_count = 0;
    g_done_head = 0;
    g_done_count = 0;
    memset(g_results, 0, sizeof(g_results));
    g_next_request_id = 1;
}

static void store_result_locked(uint64_t request_id, uint32_t length, int32_t status) {
    for (int i = 0; i < MAX_RESULTS; ++i) {
        if (!g_results[i].used) {
            g_results[i].used = true;
            g_results[i].request_id = request_id;
            g_results[i].result.length = length;
            g_results[i].result.status = status;
            return;
        }
    }
}

static bool pop_task_locked(InflateTask* out) {
    if (g_task_count <= 0) {
        return false;
    }
    *out = g_task_q[g_task_head];
    g_task_head = (g_task_head + 1) % MAX_QUEUE;
    g_task_count--;
    return true;
}

static void push_done_locked(uint64_t request_id) {
    if (g_done_count >= MAX_QUEUE) {
        return;
    }
    int tail = (g_done_head + g_done_count) % MAX_QUEUE;
    g_done_q[tail] = request_id;
    g_done_count++;
}

static void* zlib_worker_main(void* arg) {
    (void)arg;

    for (;;) {
        InflateTask task;
        pthread_mutex_lock(&g_mutex);
        while (!g_stop_requested && g_task_count == 0) {
            pthread_cond_wait(&g_task_cv, &g_mutex);
        }
        if (g_stop_requested) {
            pthread_mutex_unlock(&g_mutex);
            break;
        }
        if (!pop_task_locked(&task)) {
            pthread_mutex_unlock(&g_mutex);
            continue;
        }
        pthread_mutex_unlock(&g_mutex);

        uLongf decompressed_length = task.dst_length;
        if (g_test_delay_usec > 0) {
            struct timespec req;
            req.tv_sec = (time_t)(g_test_delay_usec / 1000000ull);
            req.tv_nsec = (long)((g_test_delay_usec % 1000000ull) * 1000ull);
            nanosleep(&req, NULL);
        }

        int ret = uncompress((Bytef*)task.dst, &decompressed_length,
                             (const Bytef*)task.src, task.src_length);

        int32_t status = ORBIS_ZLIB_ERROR_FATAL;
        if (ret == Z_BUF_ERROR) {
            status = ORBIS_ZLIB_ERROR_NOSPACE;
        } else if (ret == Z_OK) {
            status = ORBIS_OK;
        }

        pthread_mutex_lock(&g_mutex);
        store_result_locked(task.request_id, (uint32_t)decompressed_length, status);
        push_done_locked(task.request_id);
        pthread_mutex_unlock(&g_mutex);
        pthread_cond_signal(&g_done_cv);
    }

    return NULL;
}

int elisa_zlib_async_initialize(const void* buffer, uint32_t length) {
    (void)buffer;
    (void)length;

    pthread_mutex_lock(&g_mutex);
    if (g_thread_started) {
        pthread_mutex_unlock(&g_mutex);
        return ORBIS_ZLIB_ERROR_ALREADY_INITIALIZED;
    }
    clear_state_locked();
    g_stop_requested = false;
    pthread_mutex_unlock(&g_mutex);

    if (pthread_create(&g_thread, NULL, zlib_worker_main, NULL) != 0) {
        return ORBIS_ZLIB_ERROR_FATAL;
    }

    pthread_mutex_lock(&g_mutex);
    g_thread_started = true;
    pthread_mutex_unlock(&g_mutex);
    return ORBIS_OK;
}

int elisa_zlib_async_inflate(const void* src,
                             uint32_t src_len,
                             void* dst,
                             uint32_t dst_len,
                             uint64_t* request_id) {
    pthread_mutex_lock(&g_mutex);
    bool started = g_thread_started;
    pthread_mutex_unlock(&g_mutex);

    if (!started) {
        return ORBIS_ZLIB_ERROR_NOT_INITIALIZED;
    }
    if (!src || !src_len || !dst || !dst_len || !request_id || dst_len > KB_64 ||
        (dst_len % KB_2) != 0) {
        return ORBIS_ZLIB_ERROR_INVALID;
    }

    pthread_mutex_lock(&g_mutex);
    if (g_task_count >= MAX_QUEUE) {
        pthread_mutex_unlock(&g_mutex);
        return ORBIS_ZLIB_ERROR_FATAL;
    }

    uint64_t id = g_next_request_id++;
    *request_id = id;
    int tail = (g_task_head + g_task_count) % MAX_QUEUE;
    g_task_q[tail].request_id = id;
    g_task_q[tail].src = src;
    g_task_q[tail].src_length = src_len;
    g_task_q[tail].dst = dst;
    g_task_q[tail].dst_length = dst_len;
    g_task_count++;
    pthread_mutex_unlock(&g_mutex);
    pthread_cond_signal(&g_task_cv);
    return ORBIS_OK;
}

int elisa_zlib_async_wait_for_done(uint64_t* request_id, const uint32_t* timeout_ms) {
    pthread_mutex_lock(&g_mutex);
    bool started = g_thread_started;
    pthread_mutex_unlock(&g_mutex);

    if (!started) {
        return ORBIS_ZLIB_ERROR_NOT_INITIALIZED;
    }
    if (!request_id) {
        return ORBIS_ZLIB_ERROR_INVALID;
    }

    pthread_mutex_lock(&g_mutex);
    if (timeout_ms) {
        if (g_done_count == 0) {
            struct timespec now;
            clock_gettime(CLOCK_REALTIME, &now);
            uint64_t nsec = (uint64_t)now.tv_nsec + (uint64_t)(*timeout_ms % 1000u) * 1000000ull;
            struct timespec abs;
            abs.tv_sec = now.tv_sec + (*timeout_ms / 1000u) + (time_t)(nsec / 1000000000ull);
            abs.tv_nsec = (long)(nsec % 1000000000ull);
            int wait_ret = pthread_cond_timedwait(&g_done_cv, &g_mutex, &abs);
            if (wait_ret != 0 && g_done_count == 0) {
                pthread_mutex_unlock(&g_mutex);
                return ORBIS_ZLIB_ERROR_TIMEDOUT;
            }
        }
    } else {
        while (g_done_count == 0) {
            pthread_cond_wait(&g_done_cv, &g_mutex);
        }
    }

    if (g_done_count == 0) {
        pthread_mutex_unlock(&g_mutex);
        return ORBIS_ZLIB_ERROR_TIMEDOUT;
    }

    *request_id = g_done_q[g_done_head];
    g_done_head = (g_done_head + 1) % MAX_QUEUE;
    g_done_count--;
    pthread_mutex_unlock(&g_mutex);
    return ORBIS_OK;
}

int elisa_zlib_async_get_result(uint64_t request_id, uint32_t* dst_length, int32_t* status) {
    pthread_mutex_lock(&g_mutex);
    bool started = g_thread_started;
    pthread_mutex_unlock(&g_mutex);

    if (!started) {
        return ORBIS_ZLIB_ERROR_NOT_INITIALIZED;
    }
    if (!dst_length || !status) {
        return ORBIS_ZLIB_ERROR_INVALID;
    }

    pthread_mutex_lock(&g_mutex);
    for (int i = 0; i < MAX_RESULTS; ++i) {
        if (g_results[i].used && g_results[i].request_id == request_id) {
            *dst_length = g_results[i].result.length;
            *status = g_results[i].result.status;
            pthread_mutex_unlock(&g_mutex);
            return ORBIS_OK;
        }
    }
    pthread_mutex_unlock(&g_mutex);
    return ORBIS_ZLIB_ERROR_NOT_FOUND;
}

int elisa_zlib_async_finalize(void) {
    pthread_mutex_lock(&g_mutex);
    if (!g_thread_started) {
        pthread_mutex_unlock(&g_mutex);
        return ORBIS_ZLIB_ERROR_NOT_INITIALIZED;
    }
    g_stop_requested = true;
    pthread_mutex_unlock(&g_mutex);

    pthread_cond_broadcast(&g_task_cv);
    pthread_join(g_thread, NULL);

    pthread_mutex_lock(&g_mutex);
    g_thread_started = false;
    pthread_mutex_unlock(&g_mutex);
    return ORBIS_OK;
}


void elisa_zlib_async_set_test_delay_usec(uint64_t usec) {
    pthread_mutex_lock(&g_mutex);
    g_test_delay_usec = usec;
    pthread_mutex_unlock(&g_mutex);
}
