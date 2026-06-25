/* SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project */
/* SPDX-License-Identifier: GPL-2.0-or-later */

/*
 * Linux native stub bridge for HLE-library and file-IO-tail FFI symbols.
 *
 * These symbols are declared  (via @link_name) in the Elisa sources
 * but have no native implementation on Linux. They are only invoked if the
 * guest actually calls the corresponding HLE service. Providing benign stubs
 * here makes the symbols resolve to real (harmless) functions instead of NULL,
 * preventing a NULL-call segfault.
 *
 * All stubs return values indicating "unavailable / empty":
 *   integer-returning -> 0 ("no user" / "no event")
 *   pointer-returning -> NULL
 *   void-returning    -> empty body
 */

#include <stddef.h>

/* ---- UserService (userservice_ffi_bridge.elisa) ---- */

/* extern ffi_userservice_get_initial_user(out_user_id: mutable int&?) -> int */
int UserServiceElisa_GetInitialUser(int *out_user_id) {
    (void)out_user_id;
    return 0;
}

/* extern ffi_userservice_get_initial_user_value() -> int */
int UserServiceElisa_GetInitialUserValue(void) {
    return 0;
}

/* extern ffi_userservice_get_login_user_id_at(index: int) -> int */
int UserServiceElisa_GetLoginUserIdAt(int index) {
    (void)index;
    return 0;
}

/* extern ffi_userservice_get_user_color_status(user_id: int) -> int */
int UserServiceElisa_GetUserColorStatus(int user_id) {
    (void)user_id;
    return 0;
}

/* extern ffi_userservice_get_user_color_value(user_id: int) -> u32 */
unsigned int UserServiceElisa_GetUserColorValue(int user_id) {
    (void)user_id;
    return 0u;
}

/* extern ffi_userservice_get_user_name_status(user_id: int, cap: usize) -> int */
int UserServiceElisa_GetUserNameStatus(int user_id, size_t cap) {
    (void)user_id;
    (void)cap;
    return 0;
}

/* extern ffi_userservice_get_user_name_byte(user_id: int, index: usize) -> u32 */
unsigned int UserServiceElisa_GetUserNameByte(int user_id, size_t index) {
    (void)user_id;
    (void)index;
    return 0u;
}

/* extern ffi_userservice_get_event_status() -> int */
int UserServiceElisa_GetEventStatus(void) {
    return 0;
}

/* extern ffi_userservice_get_event_kind_value() -> u32 */
unsigned int UserServiceElisa_GetEventKindValue(void) {
    return 0u;
}

/* extern ffi_userservice_get_event_user_id_value() -> int */
int UserServiceElisa_GetEventUserIdValue(void) {
    return 0;
}

/* extern ffi_userservice_test_push_event(event_kind: u32, user_id: int) -> int */
int UserServiceElisa_TestPushEvent(unsigned int event_kind, int user_id) {
    (void)event_kind;
    (void)user_id;
    return 0;
}

/* ---- SystemService (systemservice_ffi_bridge.elisa) ---- */

/* extern ffi_systemservice_load_exec(path: mutable u8&?, argv: mutable void&?) -> int */
int SystemServiceElisa_LoadExec(char *path, void *argv) {
    (void)path;
    (void)argv;
    return 0;
}

/* extern ffi_systemservice_receive_event_status() -> int */
int SystemServiceElisa_ReceiveEventStatus(void) {
    return 0;
}

/* extern ffi_systemservice_receive_event_type_value() -> int */
int SystemServiceElisa_ReceiveEventTypeValue(void) {
    return 0;
}

/* extern ffi_systemservice_get_status_event_count_value() -> int */
int SystemServiceElisa_GetStatusEventCountValue(void) {
    return 0;
}

/* extern ffi_systemservice_get_display_safe_area_ratio_value() -> f32 */
float SystemServiceElisa_GetDisplaySafeAreaRatioValue(void) {
    return 0.0f;
}

/* extern ffi_systemservice_test_push_event(event_type: int) -> int */
int SystemServiceElisa_TestPushEvent(int event_type) {
    (void)event_type;
    return 0;
}

/* extern ffi_systemservice_test_get_load_exec_call_count() -> int */
int SystemServiceElisa_TestGetLoadExecCallCount(void) {
    return 0;
}

/* ---- File-IO tail (common/io_file.elisa) ---- */

/* extern posix_io_unlink(path: static u8&) -> void */
void posix_io_unlink(const char *path) {
    (void)path;
}

/* extern posix_io_set_size(file: void&, size: u64) -> bool */
unsigned char posix_io_set_size(void *file, unsigned long long size) {
    (void)file;
    (void)size;
    return 0;
}

/* extern posix_io_read_string(file: void&, length: usize) -> static u8& */
const char *posix_io_read_string(void *file, size_t length) {
    (void)file;
    (void)length;
    return NULL;
}

/* extern posix_io_get_mapping(file: void&) -> uintptr */
unsigned long long posix_io_get_mapping(void *file) {
    (void)file;
    return 0ull;
}

/* extern posix_fs_directory_size(path: static u8&) -> u64 */
unsigned long long posix_fs_directory_size(const char *path) {
    (void)path;
    return 0ull;
}
