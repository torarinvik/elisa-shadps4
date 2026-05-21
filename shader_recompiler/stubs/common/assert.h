// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/logging/log.h"

void assert_fail_impl();
[[noreturn]] void unreachable_impl();
void assert_fail_debug_msg(const char* msg);

#define ASSERT(_a_)                                                                                \
    do {                                                                                           \
        if (!(_a_)) [[unlikely]] {                                                                 \
            assert_fail_impl();                                                                    \
        }                                                                                          \
    } while (false)

#define ASSERT_MSG(_a_, ...)                                                                       \
    do {                                                                                           \
        if (!(_a_)) [[unlikely]] {                                                                 \
            assert_fail_impl();                                                                    \
        }                                                                                          \
    } while (false)

#define UNREACHABLE()                                                                              \
    do {                                                                                           \
        unreachable_impl();                                                                        \
    } while (0)

#define UNREACHABLE_MSG(...)                                                                       \
    do {                                                                                           \
        unreachable_impl();                                                                        \
    } while (0)

#define DEBUG_ASSERT(_a_) ASSERT(_a_)
#define DEBUG_ASSERT_MSG(_a_, ...) ASSERT_MSG(_a_, __VA_ARGS__)
#define UNIMPLEMENTED() ASSERT_MSG(false, "Unimplemented code!")
#define UNIMPLEMENTED_MSG(...) ASSERT_MSG(false, __VA_ARGS__)
#define UNIMPLEMENTED_IF(cond) ASSERT_MSG(!(cond), "Unimplemented code!")
#define UNIMPLEMENTED_IF_MSG(cond, ...) ASSERT_MSG(!(cond), __VA_ARGS__)
#define ASSERT_OR_EXECUTE(_a_, _b_)                                                                \
    do {                                                                                           \
        if (!(_a_)) [[unlikely]] {                                                                 \
            _b_                                                                                    \
        }                                                                                          \
    } while (0)
#define ASSERT_OR_EXECUTE_MSG(_a_, _b_, ...) ASSERT_OR_EXECUTE(_a_, _b_)
