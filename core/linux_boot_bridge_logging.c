// SPDX-License-Identifier: GPL-2.0-or-later
// Linux native host-runtime FFI bridge.
//
// Provides native implementations for host-runtime FFI symbols that have
// Elisa  declarations but no native impl on Linux:
//   - spdlog logging shims (implemented as safe no-ops; logging disabled)
//   - posix_errno (returns the current C errno)
//   - ZYAN_SUCCESS (Zydis status success predicate)
//
// Plain C linkage; symbols are unmangled and match the bare extern names
// (no @link_name was present on any of these externs).

#include <errno.h>

// --- spdlog logging shims (no-ops; logging disabled) ---
//
// Elisa externs:
//   extern spdlog_setup(log_filename: static u8&, append: bool) -> void
//   extern spdlog_shutdown() -> void
//   extern spdlog_flush_all() -> void
//   extern spdlog_drop_all() -> void
//   extern spdlog_log(logger_name: static u8&, level: LogLevel, message: static u8&) -> void
//
// static u8&  -> const char*
// bool        -> _Bool (Elisa bool is a 1-byte value)
// LogLevel    -> int   (plain Elisa enum, default int width)

void spdlog_setup(const char *log_filename, _Bool append) {
    (void)log_filename;
    (void)append;
}

void spdlog_shutdown(void) {
}

void spdlog_flush_all(void) {
}

void spdlog_drop_all(void) {
}

void spdlog_log(const char *logger_name, int level, const char *message) {
    (void)logger_name;
    (void)level;
    (void)message;
}

// --- posix_errno ---
//
// Elisa extern: extern posix_errno() -> int
int posix_errno(void) {
    return errno;
}

// --- ZYAN_SUCCESS ---
//
// Elisa extern: extern ZYAN_SUCCESS(status: int) -> bool
//
// In Zydis, ZYAN_SUCCESS(status) is true when the high (sign) bit is clear.
// Returns _Bool to match the Elisa 1-byte bool ABI.
_Bool ZYAN_SUCCESS(int status) {
    return ((unsigned int)status & 0x80000000u) == 0u;
}
