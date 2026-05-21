// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <stdexcept>

#include "common/assert.h"

void assert_fail_impl() {
    throw std::runtime_error("assertion failed");
}

[[noreturn]] void unreachable_impl() {
    throw std::runtime_error("unreachable code");
}

void assert_fail_debug_msg(const char*) {
    throw std::runtime_error("assertion failed");
}
