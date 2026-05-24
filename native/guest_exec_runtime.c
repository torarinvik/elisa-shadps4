// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#if defined(__APPLE__) && !defined(_DARWIN_C_SOURCE)
#define _DARWIN_C_SOURCE 1
#endif

#if defined(__APPLE__) && !defined(_XOPEN_SOURCE)
#define _XOPEN_SOURCE 700
#endif

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <errno.h>
#include <dlfcn.h>
#include <signal.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#if defined(__APPLE__)
#include <libkern/OSCacheControl.h>
#include <sys/ucontext.h>
#if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
#define MAP_ANONYMOUS MAP_ANON
#endif
#elif defined(__linux__)
#include <ucontext.h>
#endif
#endif

#if !defined(_WIN32)
#if !defined(MAP_ANONYMOUS)
#if defined(MAP_ANON)
#define MAP_ANONYMOUS MAP_ANON
#else
#define MAP_ANONYMOUS 0x1000
#endif
#endif
#endif

#define ELISA_GUEST_EXEC_MAX_ARGS 33

/* ELISA_ABI_CONTRACT guest_entry x86_64 ps4_process_entry noreturn */

typedef struct ElisaGuestEntryParams {
    int32_t argc;
    uint32_t padding;
    const char* argv[ELISA_GUEST_EXEC_MAX_ARGS];
    uintptr_t entry_addr;
} ElisaGuestEntryParams;

_Static_assert(offsetof(ElisaGuestEntryParams, argv) == 8, "EntryParams argv offset");
_Static_assert(offsetof(ElisaGuestEntryParams, entry_addr) == 272, "EntryParams entry offset");
_Static_assert(sizeof(ElisaGuestEntryParams) == 280, "EntryParams size");

typedef void (*ElisaGuestExitFunc)(int32_t);
typedef uint64_t (*ElisaGuestExecTestFunc)(uint64_t, uint64_t);
typedef void (*ElisaGuestExecBoundaryFunc)(uint64_t);

#if defined(__x86_64__) && !defined(_WIN32)
__attribute__((noreturn)) static void elisa_guest_exec_enter_main_entry_asm(
    ElisaGuestEntryParams* params, ElisaGuestExitFunc exit_func);
extern uintptr_t ElisaGuestExec_CurrentStackPointerAsm(void);
extern void ElisaGuestExec_EnterMainEntryAsm(uintptr_t params, uintptr_t exit_func) __attribute__((noreturn));
extern void ElisaGuestExec_CallMainEntryAsm(uintptr_t entry, uintptr_t params, uintptr_t exit_func);
extern void ElisaGuestExec_JumpMainEntryAsm(uintptr_t entry,
                                            uintptr_t params,
                                            uintptr_t exit_func) __attribute__((noreturn));
#endif

enum {
    ELISA_GUEST_EXEC_PHASE_NONE = 0,
    ELISA_GUEST_EXEC_PHASE_FUNCTION_ENTRY = 100,
    ELISA_GUEST_EXEC_PHASE_RETURN_BAD_PARAMS = 101,
    ELISA_GUEST_EXEC_PHASE_AFTER_RESET = 110,
    ELISA_GUEST_EXEC_PHASE_SYNTHETIC_ENTRY = 120,
    ELISA_GUEST_EXEC_PHASE_SYNTHETIC_BEFORE_SIGSETJMP = 121,
    ELISA_GUEST_EXEC_PHASE_SYNTHETIC_BEFORE_MAIN_ENTRY = 122,
    ELISA_GUEST_EXEC_PHASE_BEFORE_FORK_REPORT_SETUP = 200,
    ELISA_GUEST_EXEC_PHASE_RETURN_FORK_REPORT_SETUP_FAILED = 201,
    ELISA_GUEST_EXEC_PHASE_AFTER_FORK_REPORT_SETUP = 202,
    ELISA_GUEST_EXEC_PHASE_BEFORE_FORK = 210,
    ELISA_GUEST_EXEC_PHASE_RETURN_FORK_FAILED = 211,
    ELISA_GUEST_EXEC_PHASE_CHILD_BRANCH_ENTERED = 220,
    ELISA_GUEST_EXEC_PHASE_CHILD_GUARDS_INSTALLED = 221,
    ELISA_GUEST_EXEC_PHASE_CHILD_BEFORE_SIGSETJMP = 222,
    ELISA_GUEST_EXEC_PHASE_CHILD_SIGLONGJMP = 223,
    ELISA_GUEST_EXEC_PHASE_CHILD_BEFORE_MAIN_ENTRY = 224,
    ELISA_GUEST_EXEC_PHASE_CHILD_AFTER_MAIN_ENTRY = 225,
    ELISA_GUEST_EXEC_PHASE_CHILD_ENTRY_SNAPSHOT = 226,
    ELISA_GUEST_EXEC_PHASE_SIGNAL_HANDLER_ENTERED = 300,
    ELISA_GUEST_EXEC_PHASE_ALARM_HANDLER_ENTERED = 310,
    ELISA_GUEST_EXEC_PHASE_PARENT_BRANCH_ENTERED = 400,
    ELISA_GUEST_EXEC_PHASE_PARENT_WAITPID_CHILD = 401,
    ELISA_GUEST_EXEC_PHASE_PARENT_WIFSIGNALED = 402,
    ELISA_GUEST_EXEC_PHASE_PARENT_WIFEXITED = 403,
    ELISA_GUEST_EXEC_PHASE_PARENT_WAIT_STATUS_OTHER = 404,
    ELISA_GUEST_EXEC_PHASE_PARENT_WAITPID_ERROR = 410,
    ELISA_GUEST_EXEC_PHASE_PARENT_WAITPID_EINTR = 411,
    ELISA_GUEST_EXEC_PHASE_PARENT_TIMEOUT = 420,
    ELISA_GUEST_EXEC_PHASE_NATIVE_BOUNDARY_REPORTED = 500,
    ELISA_GUEST_EXEC_PHASE_UNSUPPORTED_RETURN = 900,
};

enum {
    ELISA_GUEST_EXEC_PC_SOURCE_NONE = 0,
    ELISA_GUEST_EXEC_PC_SOURCE_SIGNAL_UCONTEXT = 1,
    ELISA_GUEST_EXEC_PC_SOURCE_FALLBACK_CHILD_SIGLONGJMP = 2,
    ELISA_GUEST_EXEC_PC_SOURCE_FALLBACK_PARENT_SILENT_CHILD = 3,
    ELISA_GUEST_EXEC_PC_SOURCE_FALLBACK_PARENT_STATUS = 4,
};

static int32_t elisa_guest_exec_last_status = 0;
static int32_t elisa_guest_exec_last_signal = 0;
static uintptr_t elisa_guest_exec_last_fault_address = 0;
static uintptr_t elisa_guest_exec_last_guest_pc = 0;
static uintptr_t elisa_guest_exec_last_guest_sp = 0;
static uintptr_t elisa_guest_exec_last_guest_bp = 0;
static uintptr_t elisa_guest_exec_last_guest_rdi = 0;
static uintptr_t elisa_guest_exec_last_guest_rsi = 0;
static uintptr_t elisa_guest_exec_last_guest_rdx = 0;
static uintptr_t elisa_guest_exec_last_guest_rcx = 0;
static uintptr_t elisa_guest_exec_last_guest_r8 = 0;
static uintptr_t elisa_guest_exec_last_guest_r9 = 0;
static uintptr_t elisa_guest_exec_last_guest_rax = 0;
static uintptr_t elisa_guest_exec_last_guest_rbx = 0;
static uintptr_t elisa_guest_exec_expected_entry_params = 0;
static uintptr_t elisa_guest_exec_expected_stack_word0 = 0;
static uintptr_t elisa_guest_exec_expected_stack_word1 = 0;
static uintptr_t elisa_guest_exec_entry_stack_word0 = 0;
static uintptr_t elisa_guest_exec_entry_stack_word1 = 0;
static uintptr_t elisa_guest_exec_entry_native_word0 = 0;
static uintptr_t elisa_guest_exec_entry_native_word1 = 0;
static uintptr_t elisa_guest_exec_fault_word0 = 0;
static uintptr_t elisa_guest_exec_fault_word1 = 0;
static uintptr_t elisa_guest_exec_signal_stack_word0 = 0;
static uintptr_t elisa_guest_exec_signal_stack_word1 = 0;
static const char* elisa_guest_exec_signal_stack_symbol0 = "";
static const char* elisa_guest_exec_signal_stack_image0 = "";
static const char* elisa_guest_exec_signal_pc_symbol = "";
static const char* elisa_guest_exec_signal_pc_image = "";
static uintptr_t elisa_guest_exec_prejump_rdi = 0;
static uintptr_t elisa_guest_exec_prejump_rsi = 0;
static uintptr_t elisa_guest_exec_prejump_rsp = 0;
static uintptr_t elisa_guest_exec_signal_guest_pc = 0;
static uintptr_t elisa_guest_exec_fallback_guest_pc = 0;
static int32_t elisa_guest_exec_signal_pc_valid = 0;
static uint32_t elisa_guest_exec_entry_native_prot = 0xffffffffu;
static uint32_t elisa_guest_exec_pc_native_prot = 0xffffffffu;
static int32_t elisa_guest_exec_last_pc_was_fallback = 0;
static int32_t elisa_guest_exec_last_pc_source = ELISA_GUEST_EXEC_PC_SOURCE_NONE;
static int32_t elisa_guest_exec_last_errno = 0;
static volatile int32_t elisa_guest_exec_last_native_phase = ELISA_GUEST_EXEC_PHASE_NONE;
static int32_t elisa_guest_exec_boundary_callback_count = 0;
static uint64_t elisa_guest_exec_boundary_callback_reason = 0;
static uint64_t elisa_guest_exec_failed_relocation_count = 0;
static int32_t elisa_guest_exec_synthetic_observed_argc = 0;
static uintptr_t elisa_guest_exec_synthetic_observed_argv0 = 0;
static uintptr_t elisa_guest_exec_synthetic_observed_entry = 0;
static uintptr_t elisa_guest_exec_synthetic_observed_params = 0;
static uintptr_t elisa_guest_exec_synthetic_observed_exit_func = 0;
static uintptr_t elisa_guest_exec_synthetic_observed_stack = 0;
static uintptr_t elisa_guest_exec_aerolib_fallback_caller_pc = 0;
static uintptr_t elisa_guest_exec_aerolib_fallback_slot_va = 0;
static uint64_t elisa_guest_exec_aerolib_fallback_callsite_bytes = 0;
static uint64_t elisa_guest_exec_aerolib_fallback_invocations = 0;
static uintptr_t elisa_guest_exec_aerolib_fallback_plt_va = 0;
static uint64_t elisa_guest_exec_aerolib_fallback_plt_bytes = 0;
static const char* elisa_guest_exec_runtime_hle_module = "";
static const char* elisa_guest_exec_runtime_hle_symbol = "";
static uintptr_t elisa_guest_exec_runtime_hle_arg0 = 0;
static uintptr_t elisa_guest_exec_runtime_hle_arg1 = 0;
static uintptr_t elisa_guest_exec_runtime_hle_arg2 = 0;
static uintptr_t elisa_guest_exec_runtime_hle_ret = 0;

typedef struct ElisaGuestExecForkReport {
    volatile int32_t boundary_reached;
    volatile int32_t boundary_reason;
    volatile int32_t execution_stage;
    volatile int32_t status;
    volatile int32_t signal;
    volatile uintptr_t fault_address;
    volatile uintptr_t guest_pc;
    volatile uintptr_t guest_sp;
    volatile uintptr_t guest_bp;
    volatile uintptr_t guest_rdi;
    volatile uintptr_t guest_rsi;
    volatile uintptr_t guest_rdx;
    volatile uintptr_t guest_rcx;
    volatile uintptr_t guest_r8;
    volatile uintptr_t guest_r9;
    volatile uintptr_t guest_rax;
    volatile uintptr_t guest_rbx;
    volatile uintptr_t expected_entry_params;
    volatile uintptr_t expected_stack_word0;
    volatile uintptr_t expected_stack_word1;
    volatile uintptr_t entry_stack_word0;
    volatile uintptr_t entry_stack_word1;
    volatile uintptr_t entry_native_word0;
    volatile uintptr_t entry_native_word1;
    volatile uintptr_t fault_word0;
    volatile uintptr_t fault_word1;
    volatile uintptr_t signal_stack_word0;
    volatile uintptr_t signal_stack_word1;
    const char* signal_stack_symbol0;
    const char* signal_stack_image0;
    const char* signal_pc_symbol;
    const char* signal_pc_image;
    volatile uintptr_t prejump_rdi;
    volatile uintptr_t prejump_rsi;
    volatile uintptr_t prejump_rsp;
    volatile uintptr_t signal_guest_pc;
    volatile uintptr_t fallback_guest_pc;
    volatile int32_t signal_pc_valid;
    volatile int32_t pc_source;
    volatile uint32_t entry_native_prot;
    volatile uint32_t pc_native_prot;
    volatile uintptr_t entry_addr;
    volatile int32_t native_phase;
    volatile int32_t pc_was_fallback;
    volatile uintptr_t aerolib_fallback_caller_pc;
    volatile uintptr_t aerolib_fallback_slot_va;
    volatile uint64_t aerolib_fallback_callsite_bytes;
    volatile uint64_t aerolib_fallback_invocations;
    volatile uintptr_t aerolib_fallback_plt_va;
    volatile uint64_t aerolib_fallback_plt_bytes;
    const char* runtime_hle_module;
    const char* runtime_hle_symbol;
    volatile uintptr_t runtime_hle_arg0;
    volatile uintptr_t runtime_hle_arg1;
    volatile uintptr_t runtime_hle_arg2;
    volatile uintptr_t runtime_hle_ret;
} ElisaGuestExecForkReport;

static ElisaGuestExecForkReport* elisa_guest_exec_fork_report = NULL;

static void elisa_guest_exec_set_native_phase(int32_t phase) {
    elisa_guest_exec_last_native_phase = phase;
    if (elisa_guest_exec_fork_report != NULL) {
        elisa_guest_exec_fork_report->native_phase = phase;
    }
}

static void elisa_guest_exec_record_fallback_pc(uintptr_t pc, int32_t source) {
    if (pc == 0) {
        return;
    }
    elisa_guest_exec_fallback_guest_pc = pc;
    elisa_guest_exec_last_guest_pc = pc;
    elisa_guest_exec_last_pc_was_fallback = 1;
    elisa_guest_exec_last_pc_source = source;
    if (elisa_guest_exec_fork_report != NULL) {
        elisa_guest_exec_fork_report->fallback_guest_pc = pc;
        elisa_guest_exec_fork_report->guest_pc = pc;
        elisa_guest_exec_fork_report->pc_source = source;
        elisa_guest_exec_fork_report->pc_was_fallback = 1;
    }
}

static int32_t elisa_guest_exec_pc_source_is_fallback(int32_t source) {
    return source == ELISA_GUEST_EXEC_PC_SOURCE_FALLBACK_CHILD_SIGLONGJMP ||
           source == ELISA_GUEST_EXEC_PC_SOURCE_FALLBACK_PARENT_SILENT_CHILD ||
           source == ELISA_GUEST_EXEC_PC_SOURCE_FALLBACK_PARENT_STATUS;
}

#define ELISA_GUEST_EXEC_WRITABLE_PAGE_CACHE 65536u
static uintptr_t elisa_guest_exec_writable_pages[ELISA_GUEST_EXEC_WRITABLE_PAGE_CACHE];
static uint32_t elisa_guest_exec_writable_page_count = 0;

static void elisa_guest_exec_clear_writable_page_cache(void) {
    memset(elisa_guest_exec_writable_pages, 0, sizeof(elisa_guest_exec_writable_pages));
    elisa_guest_exec_writable_page_count = 0;
}

typedef struct ElisaGuestExecNativeRegion {
    uintptr_t base;
    uint64_t size;
    uint32_t prot;
    uint32_t active;
} ElisaGuestExecNativeRegion;

#define ELISA_GUEST_EXEC_NATIVE_REGION_CACHE 8192u
static ElisaGuestExecNativeRegion elisa_guest_exec_native_regions[ELISA_GUEST_EXEC_NATIVE_REGION_CACHE];
static uint32_t elisa_guest_exec_native_region_count = 0;

static int32_t elisa_guest_exec_region_contains(const ElisaGuestExecNativeRegion* region,
                                                uintptr_t address, uint64_t size) {
    if (region == NULL || region->base == 0 || region->size == 0 || address < region->base) {
        return 0;
    }
    uint64_t offset = (uint64_t)(address - region->base);
    return offset <= region->size && size <= (region->size - offset);
}

static int32_t elisa_guest_exec_find_native_region(uintptr_t address, uint64_t size) {
    int32_t best = -1;
    uint64_t best_size = UINT64_MAX;
    for (uint32_t i = 0; i < elisa_guest_exec_native_region_count; ++i) {
        ElisaGuestExecNativeRegion* region = &elisa_guest_exec_native_regions[i];
        if (!region->active || !elisa_guest_exec_region_contains(region, address, size)) {
            continue;
        }
        if (region->size < best_size) {
            best = (int32_t)i;
            best_size = region->size;
        }
    }
    return best;
}

static void elisa_guest_exec_record_native_region(uintptr_t base, uint64_t size, uint32_t prot) {
    if (base == 0 || size == 0) {
        return;
    }
    int32_t existing = elisa_guest_exec_find_native_region(base, size);
    if (existing >= 0 && elisa_guest_exec_native_regions[existing].base == base &&
        elisa_guest_exec_native_regions[existing].size == size) {
        elisa_guest_exec_native_regions[existing].prot = prot;
        elisa_guest_exec_native_regions[existing].active = 1;
        return;
    }
    if (elisa_guest_exec_native_region_count >= ELISA_GUEST_EXEC_NATIVE_REGION_CACHE) {
        return;
    }
    ElisaGuestExecNativeRegion* region =
        &elisa_guest_exec_native_regions[elisa_guest_exec_native_region_count++];
    region->base = base;
    region->size = size;
    region->prot = prot;
    region->active = 1;
}

static void elisa_guest_exec_forget_native_region(uintptr_t base, uint64_t size) {
    for (uint32_t i = 0; i < elisa_guest_exec_native_region_count; ++i) {
        ElisaGuestExecNativeRegion* region = &elisa_guest_exec_native_regions[i];
        if (region->active && region->base == base && region->size == size) {
            region->active = 0;
            return;
        }
    }
}

static uint32_t elisa_guest_exec_native_prot_at(uintptr_t address, uint64_t size) {
    int32_t index = elisa_guest_exec_find_native_region(address, size);
    return index >= 0 ? elisa_guest_exec_native_regions[index].prot : 0xffffffffu;
}

static uintptr_t elisa_guest_exec_read_native_word(uintptr_t address) {
    if (address == 0) {
        return 0;
    }
    int32_t index = elisa_guest_exec_find_native_region(address, sizeof(uintptr_t));
    if (index < 0 || (elisa_guest_exec_native_regions[index].prot & 1u) == 0u) {
        return 0;
    }
    uintptr_t value = 0;
    memcpy(&value, (const void*)address, sizeof(value));
    return value;
}

static uintptr_t elisa_guest_exec_read_context_word(uintptr_t address) {
    if (address == 0) {
        return 0;
    }
    uintptr_t value = 0;
    memcpy(&value, (const void*)address, sizeof(value));
    return value;
}

static void elisa_guest_exec_resolve_context_symbol(uintptr_t address,
                                                      const char** out_symbol,
                                                      const char** out_image) {
    if (out_symbol != NULL) {
        *out_symbol = "";
    }
    if (out_image != NULL) {
        *out_image = "";
    }
#if !defined(_WIN32)
    if (address == 0) {
        return;
    }
    Dl_info info;
    memset(&info, 0, sizeof(info));
    if (dladdr((const void*)address, &info) != 0) {
        if (out_symbol != NULL && info.dli_sname != NULL) {
            *out_symbol = info.dli_sname;
        }
        if (out_image != NULL && info.dli_fname != NULL) {
            *out_image = info.dli_fname;
        }
    }
#else
    (void)address;
#endif
}

static void elisa_guest_exec_flush_instruction_cache(uintptr_t address, uint64_t size) {
    if (address == 0 || size == 0) {
        return;
    }
#if defined(__APPLE__)
    sys_icache_invalidate((void*)address, (size_t)size);
#elif defined(__GNUC__) || defined(__clang__)
    __builtin___clear_cache((char*)address, (char*)(address + (uintptr_t)size));
#endif
}

#if !defined(_WIN32)
static sigjmp_buf elisa_guest_exec_jmp;
static volatile sig_atomic_t elisa_guest_exec_guard_active = 0;
static volatile sig_atomic_t elisa_guest_exec_timeout_fired = 0;
static struct sigaction elisa_guest_exec_old_segv;
static struct sigaction elisa_guest_exec_old_bus;
static struct sigaction elisa_guest_exec_old_ill;
static struct sigaction elisa_guest_exec_old_fpe;
static struct sigaction elisa_guest_exec_old_trap;
static struct sigaction elisa_guest_exec_old_alrm;
static pthread_t elisa_guest_exec_guard_target_thread;

static uintptr_t elisa_guest_exec_extract_pc(void* uctx) {
    if (uctx == NULL) return 0;
#if defined(__x86_64__)
#if defined(__APPLE__)
    ucontext_t* uc = (ucontext_t*)uctx;
    return (uintptr_t)uc->uc_mcontext->__ss.__rip;
#elif defined(__linux__)
    ucontext_t* uc = (ucontext_t*)uctx;
    return (uintptr_t)(uintptr_t)uc->uc_mcontext.gregs[REG_RIP];
#else
    (void)uctx;
    return 0;
#endif
#else
    (void)uctx;
    return 0;
#endif
}

static int32_t elisa_guest_exec_has_signal_pc(void* uctx) {
    if (uctx == NULL) return 0;
#if defined(__x86_64__)
#if defined(__APPLE__)
    ucontext_t* uc = (ucontext_t*)uctx;
    return uc->uc_mcontext != NULL ? 1 : 0;
#elif defined(__linux__)
    return 1;
#else
    (void)uctx;
    return 0;
#endif
#else
    (void)uctx;
    return 0;
#endif
}

static uintptr_t elisa_guest_exec_extract_sp(void* uctx) {
    if (uctx == NULL) return 0;
#if defined(__x86_64__)
#if defined(__APPLE__)
    ucontext_t* uc = (ucontext_t*)uctx;
    return (uintptr_t)uc->uc_mcontext->__ss.__rsp;
#elif defined(__linux__)
    ucontext_t* uc = (ucontext_t*)uctx;
    return (uintptr_t)(uintptr_t)uc->uc_mcontext.gregs[REG_RSP];
#else
    (void)uctx;
    return 0;
#endif
#else
    (void)uctx;
    return 0;
#endif
}

static uintptr_t elisa_guest_exec_extract_bp(void* uctx) {
    if (uctx == NULL) return 0;
#if defined(__x86_64__)
#if defined(__APPLE__)
    ucontext_t* uc = (ucontext_t*)uctx;
    return (uintptr_t)uc->uc_mcontext->__ss.__rbp;
#elif defined(__linux__)
    ucontext_t* uc = (ucontext_t*)uctx;
    return (uintptr_t)(uintptr_t)uc->uc_mcontext.gregs[REG_RBP];
#else
    (void)uctx;
    return 0;
#endif
#else
    (void)uctx;
    return 0;
#endif
}

static uintptr_t elisa_guest_exec_extract_greg(void* uctx, int reg) {
    if (uctx == NULL) return 0;
#if defined(__x86_64__)
#if defined(__APPLE__)
    ucontext_t* uc = (ucontext_t*)uctx;
    switch (reg) {
    case 0: return (uintptr_t)uc->uc_mcontext->__ss.__rdi;
    case 1: return (uintptr_t)uc->uc_mcontext->__ss.__rsi;
    case 2: return (uintptr_t)uc->uc_mcontext->__ss.__rax;
    case 3: return (uintptr_t)uc->uc_mcontext->__ss.__rbx;
    case 4: return (uintptr_t)uc->uc_mcontext->__ss.__rdx;
    case 5: return (uintptr_t)uc->uc_mcontext->__ss.__rcx;
    case 6: return (uintptr_t)uc->uc_mcontext->__ss.__r8;
    case 7: return (uintptr_t)uc->uc_mcontext->__ss.__r9;
    default: return 0;
    }
#elif defined(__linux__)
    ucontext_t* uc = (ucontext_t*)uctx;
    switch (reg) {
    case 0: return (uintptr_t)uc->uc_mcontext.gregs[REG_RDI];
    case 1: return (uintptr_t)uc->uc_mcontext.gregs[REG_RSI];
    case 2: return (uintptr_t)uc->uc_mcontext.gregs[REG_RAX];
    case 3: return (uintptr_t)uc->uc_mcontext.gregs[REG_RBX];
    case 4: return (uintptr_t)uc->uc_mcontext.gregs[REG_RDX];
    case 5: return (uintptr_t)uc->uc_mcontext.gregs[REG_RCX];
    case 6: return (uintptr_t)uc->uc_mcontext.gregs[REG_R8];
    case 7: return (uintptr_t)uc->uc_mcontext.gregs[REG_R9];
    default: return 0;
    }
#else
    (void)reg;
    return 0;
#endif
#else
    (void)reg;
    return 0;
#endif
}

static void elisa_guest_exec_signal_handler(int sig, siginfo_t* info, void* uctx) {
    (void)uctx;
    elisa_guest_exec_set_native_phase(ELISA_GUEST_EXEC_PHASE_SIGNAL_HANDLER_ENTERED);
    elisa_guest_exec_last_status = -sig;
    elisa_guest_exec_last_signal = sig;
    elisa_guest_exec_last_fault_address = info != NULL ? (uintptr_t)info->si_addr : 0;
    elisa_guest_exec_signal_pc_valid = elisa_guest_exec_has_signal_pc(uctx);
    elisa_guest_exec_signal_guest_pc = elisa_guest_exec_extract_pc(uctx);
    elisa_guest_exec_fallback_guest_pc = 0;
    elisa_guest_exec_last_pc_was_fallback = 0;
    elisa_guest_exec_last_pc_source = elisa_guest_exec_signal_pc_valid != 0 ?
        ELISA_GUEST_EXEC_PC_SOURCE_SIGNAL_UCONTEXT : ELISA_GUEST_EXEC_PC_SOURCE_NONE;
    elisa_guest_exec_last_guest_pc = elisa_guest_exec_signal_guest_pc;
    elisa_guest_exec_last_guest_sp = elisa_guest_exec_extract_sp(uctx);
    elisa_guest_exec_last_guest_bp = elisa_guest_exec_extract_bp(uctx);
    elisa_guest_exec_last_guest_rdi = elisa_guest_exec_extract_greg(uctx, 0);
    elisa_guest_exec_last_guest_rsi = elisa_guest_exec_extract_greg(uctx, 1);
    elisa_guest_exec_last_guest_rax = elisa_guest_exec_extract_greg(uctx, 2);
    elisa_guest_exec_last_guest_rbx = elisa_guest_exec_extract_greg(uctx, 3);
    elisa_guest_exec_last_guest_rdx = elisa_guest_exec_extract_greg(uctx, 4);
    elisa_guest_exec_last_guest_rcx = elisa_guest_exec_extract_greg(uctx, 5);
    elisa_guest_exec_last_guest_r8 = elisa_guest_exec_extract_greg(uctx, 6);
    elisa_guest_exec_last_guest_r9 = elisa_guest_exec_extract_greg(uctx, 7);
    elisa_guest_exec_pc_native_prot =
        elisa_guest_exec_native_prot_at(elisa_guest_exec_last_guest_pc, sizeof(uintptr_t));
    elisa_guest_exec_fault_word0 = elisa_guest_exec_read_native_word(elisa_guest_exec_last_guest_pc);
    elisa_guest_exec_fault_word1 =
        elisa_guest_exec_read_native_word(elisa_guest_exec_last_guest_pc + sizeof(uintptr_t));
    elisa_guest_exec_signal_stack_word0 =
        elisa_guest_exec_read_context_word(elisa_guest_exec_last_guest_sp);
    elisa_guest_exec_signal_stack_word1 =
        elisa_guest_exec_read_context_word(elisa_guest_exec_last_guest_sp + sizeof(uintptr_t));
    elisa_guest_exec_resolve_context_symbol(elisa_guest_exec_signal_stack_word0,
                                            &elisa_guest_exec_signal_stack_symbol0,
                                            &elisa_guest_exec_signal_stack_image0);
    elisa_guest_exec_resolve_context_symbol(elisa_guest_exec_last_guest_pc,
                                            &elisa_guest_exec_signal_pc_symbol,
                                            &elisa_guest_exec_signal_pc_image);
    if (elisa_guest_exec_fork_report != NULL) {
        elisa_guest_exec_fork_report->status = elisa_guest_exec_last_status;
        elisa_guest_exec_fork_report->signal = sig;
        elisa_guest_exec_fork_report->fault_address = elisa_guest_exec_last_fault_address;
        elisa_guest_exec_fork_report->guest_pc = elisa_guest_exec_last_guest_pc;
        elisa_guest_exec_fork_report->guest_sp = elisa_guest_exec_last_guest_sp;
        elisa_guest_exec_fork_report->guest_bp = elisa_guest_exec_last_guest_bp;
        elisa_guest_exec_fork_report->guest_rdi = elisa_guest_exec_last_guest_rdi;
        elisa_guest_exec_fork_report->guest_rsi = elisa_guest_exec_last_guest_rsi;
        elisa_guest_exec_fork_report->guest_rdx = elisa_guest_exec_last_guest_rdx;
        elisa_guest_exec_fork_report->guest_rcx = elisa_guest_exec_last_guest_rcx;
        elisa_guest_exec_fork_report->guest_r8 = elisa_guest_exec_last_guest_r8;
        elisa_guest_exec_fork_report->guest_r9 = elisa_guest_exec_last_guest_r9;
        elisa_guest_exec_fork_report->guest_rax = elisa_guest_exec_last_guest_rax;
        elisa_guest_exec_fork_report->guest_rbx = elisa_guest_exec_last_guest_rbx;
        elisa_guest_exec_fork_report->expected_entry_params = elisa_guest_exec_expected_entry_params;
        elisa_guest_exec_fork_report->expected_stack_word0 = elisa_guest_exec_expected_stack_word0;
        elisa_guest_exec_fork_report->expected_stack_word1 = elisa_guest_exec_expected_stack_word1;
        elisa_guest_exec_fork_report->entry_stack_word0 = elisa_guest_exec_entry_stack_word0;
        elisa_guest_exec_fork_report->entry_stack_word1 = elisa_guest_exec_entry_stack_word1;
        elisa_guest_exec_fork_report->fault_word0 = elisa_guest_exec_fault_word0;
        elisa_guest_exec_fork_report->fault_word1 = elisa_guest_exec_fault_word1;
        elisa_guest_exec_fork_report->signal_stack_word0 = elisa_guest_exec_signal_stack_word0;
        elisa_guest_exec_fork_report->signal_stack_word1 = elisa_guest_exec_signal_stack_word1;
        elisa_guest_exec_fork_report->signal_stack_symbol0 = elisa_guest_exec_signal_stack_symbol0;
        elisa_guest_exec_fork_report->signal_stack_image0 = elisa_guest_exec_signal_stack_image0;
        elisa_guest_exec_fork_report->signal_pc_symbol = elisa_guest_exec_signal_pc_symbol;
        elisa_guest_exec_fork_report->signal_pc_image = elisa_guest_exec_signal_pc_image;
        elisa_guest_exec_fork_report->prejump_rdi = elisa_guest_exec_prejump_rdi;
        elisa_guest_exec_fork_report->prejump_rsi = elisa_guest_exec_prejump_rsi;
        elisa_guest_exec_fork_report->prejump_rsp = elisa_guest_exec_prejump_rsp;
        elisa_guest_exec_fork_report->signal_guest_pc = elisa_guest_exec_signal_guest_pc;
        elisa_guest_exec_fork_report->fallback_guest_pc = elisa_guest_exec_fallback_guest_pc;
        elisa_guest_exec_fork_report->signal_pc_valid = elisa_guest_exec_signal_pc_valid;
        elisa_guest_exec_fork_report->pc_source = elisa_guest_exec_last_pc_source;
        elisa_guest_exec_fork_report->pc_was_fallback = elisa_guest_exec_last_pc_was_fallback;
        elisa_guest_exec_fork_report->pc_native_prot = elisa_guest_exec_pc_native_prot;
        if (elisa_guest_exec_fork_report->execution_stage < 6) {
            elisa_guest_exec_fork_report->execution_stage = 6;
        }
    }
    if (elisa_guest_exec_guard_active) {
        siglongjmp(elisa_guest_exec_jmp, 1);
    }
}

static void elisa_guest_exec_alarm_handler(int sig) {
    (void)sig;
    elisa_guest_exec_set_native_phase(ELISA_GUEST_EXEC_PHASE_ALARM_HANDLER_ENTERED);
    elisa_guest_exec_timeout_fired = 1;
    elisa_guest_exec_last_status = -3;
    elisa_guest_exec_last_signal = SIGALRM;
    if (elisa_guest_exec_fork_report != NULL) {
        elisa_guest_exec_fork_report->status = -3;
        elisa_guest_exec_fork_report->signal = SIGALRM;
        if (elisa_guest_exec_fork_report->execution_stage < 4) {
            elisa_guest_exec_fork_report->execution_stage = 4;
        }
    }
    if (elisa_guest_exec_guard_active) {
        siglongjmp(elisa_guest_exec_jmp, 1);
    }
}

static void __attribute__((unused)) elisa_guest_exec_install_guards(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = elisa_guest_exec_signal_handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, &elisa_guest_exec_old_segv);
    sigaction(SIGBUS, &sa, &elisa_guest_exec_old_bus);
    sigaction(SIGILL, &sa, &elisa_guest_exec_old_ill);
    sigaction(SIGFPE, &sa, &elisa_guest_exec_old_fpe);
    sigaction(SIGTRAP, &sa, &elisa_guest_exec_old_trap);
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = elisa_guest_exec_alarm_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGALRM, &sa, &elisa_guest_exec_old_alrm);
}

static void __attribute__((unused)) elisa_guest_exec_restore_guards(void) {
    sigaction(SIGSEGV, &elisa_guest_exec_old_segv, NULL);
    sigaction(SIGBUS, &elisa_guest_exec_old_bus, NULL);
    sigaction(SIGILL, &elisa_guest_exec_old_ill, NULL);
    sigaction(SIGFPE, &elisa_guest_exec_old_fpe, NULL);
    sigaction(SIGTRAP, &elisa_guest_exec_old_trap, NULL);
    sigaction(SIGALRM, &elisa_guest_exec_old_alrm, NULL);
}

typedef struct ElisaGuestExecWatchdogArgs {
    uint32_t timeout_ms;
    pthread_t target;
} ElisaGuestExecWatchdogArgs;

static void* elisa_guest_exec_watchdog_main(void* raw) {
    ElisaGuestExecWatchdogArgs* args = (ElisaGuestExecWatchdogArgs*)raw;
    if (args == NULL) {
        return NULL;
    }
    uint32_t timeout_ms = args->timeout_ms;
    pthread_t target = args->target;
    free(args);
    if (timeout_ms == 0) {
        return NULL;
    }
    usleep((useconds_t)timeout_ms * 1000u);
    if (elisa_guest_exec_guard_active) {
        pthread_kill(target, SIGALRM);
    }
    return NULL;
}

static void __attribute__((unused)) elisa_guest_exec_start_watchdog(uint32_t timeout_ms) {
    if (timeout_ms == 0) {
        return;
    }
    ElisaGuestExecWatchdogArgs* args = (ElisaGuestExecWatchdogArgs*)malloc(sizeof(ElisaGuestExecWatchdogArgs));
    if (args == NULL) {
        return;
    }
    args->timeout_ms = timeout_ms;
    args->target = elisa_guest_exec_guard_target_thread;
    pthread_t watchdog;
    if (pthread_create(&watchdog, NULL, elisa_guest_exec_watchdog_main, args) == 0) {
        pthread_detach(watchdog);
    } else {
        free(args);
    }
}
#endif

static uintptr_t elisa_guest_exec_current_stack_pointer(void) {
#if defined(__x86_64__) && !defined(_WIN32)
    return ElisaGuestExec_CurrentStackPointerAsm();
#else
    return 0;
#endif
}

static void elisa_guest_exec_reset_fork_report(void) {
    if (elisa_guest_exec_fork_report != NULL) {
        memset((void*)elisa_guest_exec_fork_report, 0, sizeof(*elisa_guest_exec_fork_report));
    }
}

static int32_t __attribute__((unused)) elisa_guest_exec_ensure_fork_report(void) {
#if defined(_WIN32)
    return 0;
#else
    if (elisa_guest_exec_fork_report != NULL) {
        elisa_guest_exec_reset_fork_report();
        return 0;
    }
    void* mapped = mmap(NULL, sizeof(ElisaGuestExecForkReport), PROT_READ | PROT_WRITE,
                        MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (mapped == MAP_FAILED) {
        elisa_guest_exec_fork_report = NULL;
        return -1;
    }
    elisa_guest_exec_fork_report = (ElisaGuestExecForkReport*)mapped;
    elisa_guest_exec_reset_fork_report();
    return 0;
#endif
}

void ElisaGuestExec_RecordRuntimeHle(const char* module, const char* symbol,
                                      uintptr_t arg0, uintptr_t arg1,
                                      uintptr_t arg2, uintptr_t ret) {
    elisa_guest_exec_runtime_hle_module = module != NULL ? module : "";
    elisa_guest_exec_runtime_hle_symbol = symbol != NULL ? symbol : "";
    elisa_guest_exec_runtime_hle_arg0 = arg0;
    elisa_guest_exec_runtime_hle_arg1 = arg1;
    elisa_guest_exec_runtime_hle_arg2 = arg2;
    elisa_guest_exec_runtime_hle_ret = ret;
    if (elisa_guest_exec_fork_report != NULL) {
        elisa_guest_exec_fork_report->runtime_hle_module = elisa_guest_exec_runtime_hle_module;
        elisa_guest_exec_fork_report->runtime_hle_symbol = elisa_guest_exec_runtime_hle_symbol;
        elisa_guest_exec_fork_report->runtime_hle_arg0 = arg0;
        elisa_guest_exec_fork_report->runtime_hle_arg1 = arg1;
        elisa_guest_exec_fork_report->runtime_hle_arg2 = arg2;
        elisa_guest_exec_fork_report->runtime_hle_ret = ret;
    }
}

const char* ElisaGuestExec_RuntimeHleModule(void) {
    return elisa_guest_exec_runtime_hle_module != NULL ? elisa_guest_exec_runtime_hle_module : "";
}

const char* ElisaGuestExec_RuntimeHleSymbol(void) {
    return elisa_guest_exec_runtime_hle_symbol != NULL ? elisa_guest_exec_runtime_hle_symbol : "";
}

uintptr_t ElisaGuestExec_RuntimeHleArg0(void) { return elisa_guest_exec_runtime_hle_arg0; }
uintptr_t ElisaGuestExec_RuntimeHleArg1(void) { return elisa_guest_exec_runtime_hle_arg1; }
uintptr_t ElisaGuestExec_RuntimeHleArg2(void) { return elisa_guest_exec_runtime_hle_arg2; }
uintptr_t ElisaGuestExec_RuntimeHleRet(void) { return elisa_guest_exec_runtime_hle_ret; }

void ElisaGuestExec_ReportFirstBoundary(int32_t reason) {
    elisa_guest_exec_set_native_phase(ELISA_GUEST_EXEC_PHASE_NATIVE_BOUNDARY_REPORTED);
    if (elisa_guest_exec_fork_report != NULL) {
        elisa_guest_exec_fork_report->boundary_reached = 1;
        if (elisa_guest_exec_fork_report->boundary_reason == 0) {
            elisa_guest_exec_fork_report->boundary_reason = reason;
        }
        elisa_guest_exec_fork_report->execution_stage = 5;
    }
}

// Native AeroLib fallback stub. Unresolved JUMP_SLOT/GLOB_DAT relocations and
// NID-table fallbacks are patched to point directly at this function, so when
// the guest code does `call qword ptr [GOT_slot]` it lands here and
// __builtin_return_address(0) is exactly the guest call site. We record the
// first caller PC (and, when the call site uses the standard RIP-relative
// `FF 15 disp32` encoding, the GOT slot virtual address it dispatched through)
// into the shared fork report so the parent can attribute the crash to a
// specific unimplemented import. Returns 0 just like the previous stub.
uint64_t ElisaGuestExec_AeroLibFallbackStubImpl(void) {
    uintptr_t caller_pc = (uintptr_t)__builtin_return_address(0);
    if (elisa_guest_exec_fork_report != NULL) {
        elisa_guest_exec_fork_report->aerolib_fallback_invocations++;
        if (elisa_guest_exec_fork_report->aerolib_fallback_caller_pc == 0 && caller_pc != 0) {
            elisa_guest_exec_fork_report->aerolib_fallback_caller_pc = caller_pc;
            const uint8_t* p = (const uint8_t*)caller_pc;
            uint64_t raw = 0;
            for (int i = 1; i <= 8; ++i) {
                raw = (raw << 8) | (uint64_t)p[-i];
            }
            elisa_guest_exec_fork_report->aerolib_fallback_callsite_bytes = raw;
            uintptr_t slot_va = 0;
            // Case 1: `call qword ptr [rip+disp32]` (FF 15 disp32), 6 bytes
            // ending at caller_pc. The GOT slot is the RIP-relative target.
            if (p[-6] == 0xFF && p[-5] == 0x15) {
                int32_t disp = (int32_t)((uint32_t)p[-4] |
                                         ((uint32_t)p[-3] << 8) |
                                         ((uint32_t)p[-2] << 16) |
                                         ((uint32_t)p[-1] << 24));
                slot_va = (uintptr_t)((int64_t)caller_pc + (int64_t)disp);
            } else if (p[-5] == 0xE8) {
                // Case 2: `call rel32` (E8 disp32), 5 bytes ending at
                // caller_pc, targeting a PLT thunk. Follow the thunk: a
                // standard PLT entry is `jmp qword ptr [rip+disp32]`
                // (FF 25 disp32); the GOT slot is its RIP-relative target.
                int32_t rel = (int32_t)((uint32_t)p[-4] |
                                        ((uint32_t)p[-3] << 8) |
                                        ((uint32_t)p[-2] << 16) |
                                        ((uint32_t)p[-1] << 24));
                uintptr_t plt = (uintptr_t)((int64_t)caller_pc + (int64_t)rel);
                elisa_guest_exec_fork_report->aerolib_fallback_plt_va = plt;
                const uint8_t* q = (const uint8_t*)plt;
                uint64_t plt_raw = 0;
                for (int i = 7; i >= 0; --i) {
                    plt_raw = (plt_raw << 8) | (uint64_t)q[i];
                }
                elisa_guest_exec_fork_report->aerolib_fallback_plt_bytes = plt_raw;
                if (q[0] == 0xFF && q[1] == 0x25) {
                    int32_t disp = (int32_t)((uint32_t)q[2] |
                                             ((uint32_t)q[3] << 8) |
                                             ((uint32_t)q[4] << 16) |
                                             ((uint32_t)q[5] << 24));
                    // RIP for the jmp is the address after its 6 bytes.
                    slot_va = (uintptr_t)((int64_t)plt + 6 + (int64_t)disp);
                }
            }
            elisa_guest_exec_fork_report->aerolib_fallback_slot_va = slot_va;
        }
    }
    ElisaGuestExec_ReportFirstBoundary(2 /* GUEST_EXEC_BOUNDARY_REASON_AEROLIB_FALLBACK */);
    return 0;
}

uintptr_t ElisaGuestExec_AeroLibFallbackStubAddress(void) {
    return (uintptr_t)(void*)&ElisaGuestExec_AeroLibFallbackStubImpl;
}

uintptr_t ElisaGuestExec_AeroLibFallbackCallerPc(void) {
    return elisa_guest_exec_aerolib_fallback_caller_pc;
}

uintptr_t ElisaGuestExec_AeroLibFallbackSlotVa(void) {
    return elisa_guest_exec_aerolib_fallback_slot_va;
}

uint64_t ElisaGuestExec_AeroLibFallbackCallsiteBytes(void) {
    return elisa_guest_exec_aerolib_fallback_callsite_bytes;
}

uint64_t ElisaGuestExec_AeroLibFallbackInvocations(void) {
    return elisa_guest_exec_aerolib_fallback_invocations;
}

uintptr_t ElisaGuestExec_AeroLibFallbackPltVa(void) {
    return elisa_guest_exec_aerolib_fallback_plt_va;
}

uint64_t ElisaGuestExec_AeroLibFallbackPltBytes(void) {
    return elisa_guest_exec_aerolib_fallback_plt_bytes;
}

int32_t ElisaGuestExec_ReportedBoundaryReached(void) {
    return elisa_guest_exec_fork_report != NULL ? elisa_guest_exec_fork_report->boundary_reached : 0;
}

int32_t ElisaGuestExec_ReportedBoundaryReason(void) {
    return elisa_guest_exec_fork_report != NULL ? elisa_guest_exec_fork_report->boundary_reason : 0;
}

int32_t ElisaGuestExec_ReportedExecutionStage(void) {
    return elisa_guest_exec_fork_report != NULL ? elisa_guest_exec_fork_report->execution_stage : 0;
}

int32_t ElisaGuestExec_ReportedStatus(void) {
    return elisa_guest_exec_fork_report != NULL ? elisa_guest_exec_fork_report->status : 0;
}

int32_t ElisaGuestExec_ReportedSignal(void) {
    return elisa_guest_exec_fork_report != NULL ? elisa_guest_exec_fork_report->signal : 0;
}

uintptr_t ElisaGuestExec_ReportedFaultAddress(void) {
    return elisa_guest_exec_fork_report != NULL ? elisa_guest_exec_fork_report->fault_address : 0;
}

uintptr_t ElisaGuestExec_ReportedGuestPC(void) {
    return elisa_guest_exec_fork_report != NULL ? elisa_guest_exec_fork_report->guest_pc : 0;
}

uintptr_t ElisaGuestExec_ReportedGuestSP(void) {
    return elisa_guest_exec_fork_report != NULL ? elisa_guest_exec_fork_report->guest_sp : 0;
}

uintptr_t ElisaGuestExec_ReportedGuestBP(void) {
    return elisa_guest_exec_fork_report != NULL ? elisa_guest_exec_fork_report->guest_bp : 0;
}

int32_t ElisaGuestExec_ReportedNativePhase(void) {
    return elisa_guest_exec_fork_report != NULL ? elisa_guest_exec_fork_report->native_phase : 0;
}

static void __attribute__((unused)) elisa_guest_exec_absorb_fork_report(void) {
    if (elisa_guest_exec_fork_report == NULL) {
        return;
    }
    if (elisa_guest_exec_fork_report->status == 0 &&
        elisa_guest_exec_fork_report->signal == 0 &&
        elisa_guest_exec_fork_report->execution_stage >= 4) {
        elisa_guest_exec_fork_report->status = -11;
        if (elisa_guest_exec_fork_report->guest_pc == 0 &&
            elisa_guest_exec_fork_report->signal_pc_valid == 0) {
            elisa_guest_exec_record_fallback_pc(
                elisa_guest_exec_fork_report->entry_addr,
                ELISA_GUEST_EXEC_PC_SOURCE_FALLBACK_PARENT_SILENT_CHILD);
        }
    }
    if (elisa_guest_exec_fork_report->status < 0 &&
        elisa_guest_exec_fork_report->guest_pc == 0 &&
        elisa_guest_exec_fork_report->signal_pc_valid == 0 &&
        elisa_guest_exec_fork_report->entry_addr != 0) {
        elisa_guest_exec_record_fallback_pc(
            elisa_guest_exec_fork_report->entry_addr,
            ELISA_GUEST_EXEC_PC_SOURCE_FALLBACK_PARENT_STATUS);
    }
    if (elisa_guest_exec_fork_report->status != 0) {
        elisa_guest_exec_last_status = elisa_guest_exec_fork_report->status;
    }
    if (elisa_guest_exec_fork_report->signal != 0) {
        elisa_guest_exec_last_signal = elisa_guest_exec_fork_report->signal;
    }
    if (elisa_guest_exec_fork_report->fault_address != 0 ||
        elisa_guest_exec_fork_report->status < 0) {
        elisa_guest_exec_last_fault_address = elisa_guest_exec_fork_report->fault_address;
    }
    if (elisa_guest_exec_fork_report->guest_pc != 0) {
        elisa_guest_exec_last_guest_pc = elisa_guest_exec_fork_report->guest_pc;
    }
    if (elisa_guest_exec_fork_report->guest_sp != 0) {
        elisa_guest_exec_last_guest_sp = elisa_guest_exec_fork_report->guest_sp;
    }
    if (elisa_guest_exec_fork_report->guest_bp != 0) {
        elisa_guest_exec_last_guest_bp = elisa_guest_exec_fork_report->guest_bp;
    }
    if (elisa_guest_exec_fork_report->guest_rdi != 0) {
        elisa_guest_exec_last_guest_rdi = elisa_guest_exec_fork_report->guest_rdi;
    }
    if (elisa_guest_exec_fork_report->guest_rsi != 0) {
        elisa_guest_exec_last_guest_rsi = elisa_guest_exec_fork_report->guest_rsi;
    }
    if (elisa_guest_exec_fork_report->guest_rax != 0) {
        elisa_guest_exec_last_guest_rax = elisa_guest_exec_fork_report->guest_rax;
    }
    if (elisa_guest_exec_fork_report->guest_rbx != 0) {
        elisa_guest_exec_last_guest_rbx = elisa_guest_exec_fork_report->guest_rbx;
    }
    if (elisa_guest_exec_fork_report->expected_entry_params != 0) {
        elisa_guest_exec_expected_entry_params = elisa_guest_exec_fork_report->expected_entry_params;
    }
    if (elisa_guest_exec_fork_report->expected_stack_word0 != 0 ||
        elisa_guest_exec_fork_report->expected_stack_word1 != 0) {
        elisa_guest_exec_expected_stack_word0 = elisa_guest_exec_fork_report->expected_stack_word0;
        elisa_guest_exec_expected_stack_word1 = elisa_guest_exec_fork_report->expected_stack_word1;
    }
    if (elisa_guest_exec_fork_report->entry_stack_word0 != 0 ||
        elisa_guest_exec_fork_report->entry_stack_word1 != 0) {
        elisa_guest_exec_entry_stack_word0 = elisa_guest_exec_fork_report->entry_stack_word0;
        elisa_guest_exec_entry_stack_word1 = elisa_guest_exec_fork_report->entry_stack_word1;
    }
    if (elisa_guest_exec_fork_report->prejump_rdi != 0 ||
        elisa_guest_exec_fork_report->prejump_rsi != 0 ||
        elisa_guest_exec_fork_report->prejump_rsp != 0) {
        elisa_guest_exec_prejump_rdi = elisa_guest_exec_fork_report->prejump_rdi;
        elisa_guest_exec_prejump_rsi = elisa_guest_exec_fork_report->prejump_rsi;
        elisa_guest_exec_prejump_rsp = elisa_guest_exec_fork_report->prejump_rsp;
    }
    if (elisa_guest_exec_fork_report->signal_pc_valid != 0) {
        elisa_guest_exec_signal_pc_valid = 1;
        elisa_guest_exec_signal_guest_pc = elisa_guest_exec_fork_report->signal_guest_pc;
        if (!elisa_guest_exec_fork_report->pc_was_fallback) {
            elisa_guest_exec_last_guest_pc = elisa_guest_exec_signal_guest_pc;
        }
    }
    if (elisa_guest_exec_fork_report->fallback_guest_pc != 0) {
        elisa_guest_exec_fallback_guest_pc = elisa_guest_exec_fork_report->fallback_guest_pc;
    }
    if (elisa_guest_exec_fork_report->aerolib_fallback_invocations != 0) {
        elisa_guest_exec_aerolib_fallback_invocations =
            elisa_guest_exec_fork_report->aerolib_fallback_invocations;
        elisa_guest_exec_aerolib_fallback_caller_pc =
            elisa_guest_exec_fork_report->aerolib_fallback_caller_pc;
        elisa_guest_exec_aerolib_fallback_slot_va =
            elisa_guest_exec_fork_report->aerolib_fallback_slot_va;
        elisa_guest_exec_aerolib_fallback_callsite_bytes =
            elisa_guest_exec_fork_report->aerolib_fallback_callsite_bytes;
        elisa_guest_exec_aerolib_fallback_plt_va =
            elisa_guest_exec_fork_report->aerolib_fallback_plt_va;
        elisa_guest_exec_aerolib_fallback_plt_bytes =
            elisa_guest_exec_fork_report->aerolib_fallback_plt_bytes;
    }
    if (elisa_guest_exec_fork_report->pc_source != 0) {
        elisa_guest_exec_last_pc_source = elisa_guest_exec_fork_report->pc_source;
    }
    if (elisa_guest_exec_fork_report->entry_native_word0 != 0 ||
        elisa_guest_exec_fork_report->entry_native_word1 != 0) {
        elisa_guest_exec_entry_native_word0 = elisa_guest_exec_fork_report->entry_native_word0;
        elisa_guest_exec_entry_native_word1 = elisa_guest_exec_fork_report->entry_native_word1;
    }
    if (elisa_guest_exec_fork_report->fault_word0 != 0 ||
        elisa_guest_exec_fork_report->fault_word1 != 0) {
        elisa_guest_exec_fault_word0 = elisa_guest_exec_fork_report->fault_word0;
        elisa_guest_exec_fault_word1 = elisa_guest_exec_fork_report->fault_word1;
    }
    if (elisa_guest_exec_fork_report->signal_stack_word0 != 0 ||
        elisa_guest_exec_fork_report->signal_stack_word1 != 0) {
        elisa_guest_exec_signal_stack_word0 = elisa_guest_exec_fork_report->signal_stack_word0;
        elisa_guest_exec_signal_stack_word1 = elisa_guest_exec_fork_report->signal_stack_word1;
    }
    if (elisa_guest_exec_fork_report->signal_stack_symbol0 != NULL) {
        elisa_guest_exec_signal_stack_symbol0 =
            elisa_guest_exec_fork_report->signal_stack_symbol0;
    }
    if (elisa_guest_exec_fork_report->signal_stack_image0 != NULL) {
        elisa_guest_exec_signal_stack_image0 =
            elisa_guest_exec_fork_report->signal_stack_image0;
    }
    if (elisa_guest_exec_fork_report->signal_pc_symbol != NULL) {
        elisa_guest_exec_signal_pc_symbol =
            elisa_guest_exec_fork_report->signal_pc_symbol;
    }
    if (elisa_guest_exec_fork_report->signal_pc_image != NULL) {
        elisa_guest_exec_signal_pc_image =
            elisa_guest_exec_fork_report->signal_pc_image;
    }
    if (elisa_guest_exec_fork_report->entry_native_prot != 0) {
        elisa_guest_exec_entry_native_prot = elisa_guest_exec_fork_report->entry_native_prot;
    }
    if (elisa_guest_exec_fork_report->pc_native_prot != 0) {
        elisa_guest_exec_pc_native_prot = elisa_guest_exec_fork_report->pc_native_prot;
    }
    if (elisa_guest_exec_fork_report->pc_was_fallback != 0) {
        elisa_guest_exec_last_pc_was_fallback = 1;
    }
    if (elisa_guest_exec_fork_report->native_phase != 0) {
        elisa_guest_exec_last_native_phase = elisa_guest_exec_fork_report->native_phase;
    }
    if (elisa_guest_exec_fork_report->runtime_hle_symbol != NULL) {
        elisa_guest_exec_runtime_hle_module = elisa_guest_exec_fork_report->runtime_hle_module;
        elisa_guest_exec_runtime_hle_symbol = elisa_guest_exec_fork_report->runtime_hle_symbol;
        elisa_guest_exec_runtime_hle_arg0 = elisa_guest_exec_fork_report->runtime_hle_arg0;
        elisa_guest_exec_runtime_hle_arg1 = elisa_guest_exec_fork_report->runtime_hle_arg1;
        elisa_guest_exec_runtime_hle_arg2 = elisa_guest_exec_fork_report->runtime_hle_arg2;
        elisa_guest_exec_runtime_hle_ret = elisa_guest_exec_fork_report->runtime_hle_ret;
    }
}

static void __attribute__((unused)) elisa_guest_exec_default_exit(int32_t code) {
    (void)code;
    for (;;) {
#if defined(_WIN32)
        Sleep(INFINITE);
#else
        pause();
#endif
    }
}

static void __attribute__((unused)) elisa_guest_exec_guarded_exit(int32_t code) {
    (void)code;
    // Match C++ ProgramExitFunc: it reports/logs the callback but returns to
    // guest code. Long-jumping here stops real games before the next actual
    // HLE/import boundary or true fault can be observed.
    elisa_guest_exec_last_status = 0;
#if !defined(_WIN32)
    if (elisa_guest_exec_fork_report != NULL) {
        if (elisa_guest_exec_fork_report->execution_stage < 4) {
            elisa_guest_exec_fork_report->execution_stage = 4;
        }
    }
#endif
}

#if defined(__x86_64__) && !defined(_WIN32)
__attribute__((noreturn)) static void elisa_guest_exec_enter_main_entry_asm(
    ElisaGuestEntryParams* params, ElisaGuestExitFunc exit_func) {
    ElisaGuestExec_EnterMainEntryAsm((uintptr_t)params, (uintptr_t)exit_func);
}

static void elisa_guest_exec_call_main_entry(ElisaGuestEntryParams* params,
                                             ElisaGuestExitFunc exit_func) {
    if (exit_func == NULL) {
        exit_func = elisa_guest_exec_default_exit;
    }
    ElisaGuestExec_CallMainEntryAsm(params->entry_addr, (uintptr_t)params, (uintptr_t)exit_func);
}


static void __attribute__((noreturn)) elisa_guest_exec_jump_main_entry(ElisaGuestEntryParams* params,
                                                                    ElisaGuestExitFunc exit_func) {
    if (exit_func == NULL) {
        exit_func = elisa_guest_exec_default_exit;
    }
    elisa_guest_exec_prejump_rdi = (uintptr_t)params;
    elisa_guest_exec_prejump_rsi = (uintptr_t)exit_func;
    elisa_guest_exec_prejump_rsp = elisa_guest_exec_current_stack_pointer();
    if (elisa_guest_exec_fork_report != NULL) {
        elisa_guest_exec_fork_report->prejump_rdi = elisa_guest_exec_prejump_rdi;
        elisa_guest_exec_fork_report->prejump_rsi = elisa_guest_exec_prejump_rsi;
        elisa_guest_exec_fork_report->prejump_rsp = elisa_guest_exec_prejump_rsp;
    }
    ElisaGuestExec_JumpMainEntryAsm(params->entry_addr, (uintptr_t)params, (uintptr_t)exit_func);
}

static void __attribute__((noreturn)) elisa_guest_exec_run_child_main_entry(
    ElisaGuestEntryParams* params) {
    elisa_guest_exec_set_native_phase(ELISA_GUEST_EXEC_PHASE_CHILD_BRANCH_ENTERED);
    elisa_guest_exec_install_guards();
    elisa_guest_exec_guard_active = 1;
    elisa_guest_exec_guard_target_thread = pthread_self();
    elisa_guest_exec_set_native_phase(ELISA_GUEST_EXEC_PHASE_CHILD_GUARDS_INSTALLED);
    if (elisa_guest_exec_fork_report != NULL) {
        elisa_guest_exec_expected_entry_params = (uintptr_t)params;
        elisa_guest_exec_expected_stack_word0 = (uintptr_t)(uint32_t)params->argc;
        elisa_guest_exec_expected_stack_word1 = (uintptr_t)params->argv[0];
        elisa_guest_exec_entry_stack_word0 = elisa_guest_exec_expected_stack_word0;
        elisa_guest_exec_entry_stack_word1 = elisa_guest_exec_expected_stack_word1;
        elisa_guest_exec_entry_native_word0 = elisa_guest_exec_read_native_word(params->entry_addr);
        elisa_guest_exec_entry_native_word1 =
            elisa_guest_exec_read_native_word(params->entry_addr + sizeof(uintptr_t));
        elisa_guest_exec_entry_native_prot =
            elisa_guest_exec_native_prot_at(params->entry_addr, sizeof(uintptr_t));
        elisa_guest_exec_fork_report->expected_entry_params = elisa_guest_exec_expected_entry_params;
        elisa_guest_exec_fork_report->expected_stack_word0 = elisa_guest_exec_expected_stack_word0;
        elisa_guest_exec_fork_report->expected_stack_word1 = elisa_guest_exec_expected_stack_word1;
        elisa_guest_exec_fork_report->entry_stack_word0 = elisa_guest_exec_entry_stack_word0;
        elisa_guest_exec_fork_report->entry_stack_word1 = elisa_guest_exec_entry_stack_word1;
        elisa_guest_exec_fork_report->entry_native_word0 = elisa_guest_exec_entry_native_word0;
        elisa_guest_exec_fork_report->entry_native_word1 = elisa_guest_exec_entry_native_word1;
        elisa_guest_exec_fork_report->entry_native_prot = elisa_guest_exec_entry_native_prot;
        elisa_guest_exec_fork_report->entry_addr = params->entry_addr;
        if (elisa_guest_exec_fork_report->execution_stage < 4) {
            elisa_guest_exec_fork_report->execution_stage = 4;
        }
    }
    elisa_guest_exec_set_native_phase(ELISA_GUEST_EXEC_PHASE_CHILD_ENTRY_SNAPSHOT);
    elisa_guest_exec_set_native_phase(ELISA_GUEST_EXEC_PHASE_CHILD_BEFORE_SIGSETJMP);
    if (sigsetjmp(elisa_guest_exec_jmp, 1) != 0) {
        elisa_guest_exec_set_native_phase(ELISA_GUEST_EXEC_PHASE_CHILD_SIGLONGJMP);
        int32_t status = elisa_guest_exec_last_status;
        int32_t sig = elisa_guest_exec_last_signal;
        if (elisa_guest_exec_fork_report != NULL) {
            if (elisa_guest_exec_fork_report->status != 0) {
                status = elisa_guest_exec_fork_report->status;
            }
            if (elisa_guest_exec_fork_report->signal != 0) {
                sig = elisa_guest_exec_fork_report->signal;
            }
        }
        if (status == 0 && sig == 0) {
            status = -11;
            elisa_guest_exec_last_status = status;
        }
        if (elisa_guest_exec_fork_report != NULL) {
            if (elisa_guest_exec_fork_report->status == 0) {
                elisa_guest_exec_fork_report->status = status;
            }
            if (elisa_guest_exec_fork_report->signal == 0) {
                elisa_guest_exec_fork_report->signal = sig;
            }
            if (elisa_guest_exec_fork_report->guest_pc == 0 &&
                elisa_guest_exec_fork_report->signal_pc_valid == 0) {
                elisa_guest_exec_record_fallback_pc(
                    params->entry_addr,
                    ELISA_GUEST_EXEC_PC_SOURCE_FALLBACK_CHILD_SIGLONGJMP);
            }
        }
        elisa_guest_exec_guard_active = 0;
        elisa_guest_exec_restore_guards();
        if (sig > 0 && sig < 128) {
            _exit(128 + sig);
        }
        if (status < 0 && -status < 128) {
            _exit(-status);
        }
        _exit(1);
    }
    elisa_guest_exec_set_native_phase(ELISA_GUEST_EXEC_PHASE_CHILD_BEFORE_MAIN_ENTRY);
    elisa_guest_exec_jump_main_entry(params, elisa_guest_exec_guarded_exit);
}
#endif

int32_t ElisaGuestExec_IsSupported(void) {
#if defined(_WIN32)
    return 0;
#elif defined(__x86_64__) || defined(_M_X64)
    return 1;
#else
    return 0;
#endif
}

int32_t ElisaGuestExec_SupportedNativeExecution(void) {
    return ElisaGuestExec_IsSupported();
}

int32_t ElisaGuestExec_HostCanRunX64Subprocess(void) {
#if defined(__x86_64__) || defined(_M_X64)
    return 1;
#elif defined(__APPLE__) && (defined(__aarch64__) || defined(_M_ARM64))
    FILE* proc = popen("/usr/bin/arch -x86_64 /usr/bin/uname -m 2>/dev/null", "r");
    if (proc == NULL) {
        return 0;
    }
    char line[64];
    memset(line, 0, sizeof(line));
    int has_x64 = 0;
    if (fgets(line, sizeof(line), proc) != NULL) {
        for (size_t i = 0; i < sizeof(line); ++i) {
            if (line[i] == '\0') {
                break;
            }
            if (line[i] == '\n' || line[i] == '\r') {
                line[i] = '\0';
                break;
            }
        }
        has_x64 = strcmp(line, "x86_64") == 0 ? 1 : 0;
    }
    int status = pclose(proc);
    if (status == -1) {
        return 0;
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        return 0;
    }
    return has_x64;
#else
    return 0;
#endif
}

const char* ElisaGuestExec_HostArchitecture(void) {
#if defined(_WIN32)
#if defined(_M_X64)
    return "x86_64";
#elif defined(_M_ARM64)
    return "arm64";
#else
    return "unknown";
#endif
#elif defined(__x86_64__) || defined(_M_X64)
    return "x86_64";
#elif defined(__aarch64__) || defined(_M_ARM64)
    return "arm64";
#elif defined(__i386__) || defined(_M_IX86)
    return "x86";
#else
    return "unknown";
#endif
}

const char* ElisaGuestExec_HostMode(void) {
#if defined(_WIN32)
    return "Windows";
#elif defined(__APPLE__)
    return "macOS";
#elif defined(__linux__)
    return "Linux";
#elif defined(__FreeBSD__)
    return "FreeBSD";
#else
    return "unknown";
#endif
}

int32_t ElisaGuestExec_LastStatus(void) {
    return elisa_guest_exec_last_status;
}

int32_t ElisaGuestExec_LastSignal(void) {
    return elisa_guest_exec_last_signal;
}

int32_t ElisaGuestExec_LastErrno(void) {
    return elisa_guest_exec_last_errno;
}

int32_t ElisaGuestExec_LastNativePhase(void) {
    if (elisa_guest_exec_fork_report != NULL && elisa_guest_exec_fork_report->native_phase != 0) {
        return elisa_guest_exec_fork_report->native_phase;
    }
    return elisa_guest_exec_last_native_phase;
}

uintptr_t ElisaGuestExec_LastFaultAddress(void) {
    return elisa_guest_exec_last_fault_address;
}

uintptr_t ElisaGuestExec_LastGuestPC(void) {
    return elisa_guest_exec_last_guest_pc;
}

uintptr_t ElisaGuestExec_LastGuestSP(void) {
    return elisa_guest_exec_last_guest_sp;
}

uintptr_t ElisaGuestExec_LastGuestBP(void) {
    return elisa_guest_exec_last_guest_bp;
}

uintptr_t ElisaGuestExec_LastGuestRDI(void) {
    return elisa_guest_exec_last_guest_rdi;
}

uintptr_t ElisaGuestExec_LastGuestRSI(void) {
    return elisa_guest_exec_last_guest_rsi;
}

uintptr_t ElisaGuestExec_LastGuestRAX(void) {
    return elisa_guest_exec_last_guest_rax;
}

uintptr_t ElisaGuestExec_LastGuestRBX(void) {
    return elisa_guest_exec_last_guest_rbx;
}

uintptr_t ElisaGuestExec_LastGuestRDX(void) {
    return elisa_guest_exec_last_guest_rdx;
}

uintptr_t ElisaGuestExec_LastGuestRCX(void) {
    return elisa_guest_exec_last_guest_rcx;
}

uintptr_t ElisaGuestExec_LastGuestR8(void) {
    return elisa_guest_exec_last_guest_r8;
}

uintptr_t ElisaGuestExec_LastGuestR9(void) {
    return elisa_guest_exec_last_guest_r9;
}

uintptr_t ElisaGuestExec_ExpectedEntryParams(void) {
    return elisa_guest_exec_expected_entry_params;
}

uintptr_t ElisaGuestExec_ExpectedStackWord0(void) {
    return elisa_guest_exec_expected_stack_word0;
}

uintptr_t ElisaGuestExec_ExpectedStackWord1(void) {
    return elisa_guest_exec_expected_stack_word1;
}

uintptr_t ElisaGuestExec_EntryStackWord0(void) {
    return elisa_guest_exec_entry_stack_word0;
}

uintptr_t ElisaGuestExec_EntryStackWord1(void) {
    return elisa_guest_exec_entry_stack_word1;
}

uintptr_t ElisaGuestExec_EntryNativeWord0(void) {
    return elisa_guest_exec_entry_native_word0;
}

uintptr_t ElisaGuestExec_EntryNativeWord1(void) {
    return elisa_guest_exec_entry_native_word1;
}

uintptr_t ElisaGuestExec_PreJumpRDI(void) {
    return elisa_guest_exec_prejump_rdi;
}

uintptr_t ElisaGuestExec_PreJumpRSI(void) {
    return elisa_guest_exec_prejump_rsi;
}

uintptr_t ElisaGuestExec_PreJumpRSP(void) {
    return elisa_guest_exec_prejump_rsp;
}

uintptr_t ElisaGuestExec_FaultWord0(void) {
    return elisa_guest_exec_fault_word0;
}

uintptr_t ElisaGuestExec_FaultWord1(void) {
    return elisa_guest_exec_fault_word1;
}

uintptr_t ElisaGuestExec_SignalStackWord0(void) {
    return elisa_guest_exec_signal_stack_word0;
}

uintptr_t ElisaGuestExec_SignalStackWord1(void) {
    return elisa_guest_exec_signal_stack_word1;
}

const char* ElisaGuestExec_SignalStackSymbol0(void) {
    return elisa_guest_exec_signal_stack_symbol0 != NULL ? elisa_guest_exec_signal_stack_symbol0 : "";
}

const char* ElisaGuestExec_SignalStackImage0(void) {
    return elisa_guest_exec_signal_stack_image0 != NULL ? elisa_guest_exec_signal_stack_image0 : "";
}

const char* ElisaGuestExec_SignalPCSymbol(void) {
    return elisa_guest_exec_signal_pc_symbol != NULL ? elisa_guest_exec_signal_pc_symbol : "";
}

const char* ElisaGuestExec_SignalPCImage(void) {
    return elisa_guest_exec_signal_pc_image != NULL ? elisa_guest_exec_signal_pc_image : "";
}

uint32_t ElisaGuestExec_EntryNativeProt(void) {
    return elisa_guest_exec_entry_native_prot;
}

uint32_t ElisaGuestExec_LastPCNativeProt(void) {
    return elisa_guest_exec_pc_native_prot;
}

int32_t ElisaGuestExec_LastPCWasFallback(void) {
    return elisa_guest_exec_last_pc_was_fallback ||
           elisa_guest_exec_pc_source_is_fallback(elisa_guest_exec_last_pc_source);
}

uintptr_t ElisaGuestExec_SignalGuestPC(void) {
    return elisa_guest_exec_signal_guest_pc;
}

uintptr_t ElisaGuestExec_FallbackGuestPC(void) {
    return elisa_guest_exec_fallback_guest_pc;
}

int32_t ElisaGuestExec_SignalPCValid(void) {
    return elisa_guest_exec_signal_pc_valid;
}

int32_t ElisaGuestExec_LastPCSource(void) {
    return elisa_guest_exec_last_pc_source;
}

uint32_t ElisaGuestExec_NativeRegionCount(void) {
    uint32_t active = 0;
    for (uint32_t i = 0; i < elisa_guest_exec_native_region_count; ++i) {
        if (elisa_guest_exec_native_regions[i].active) {
            active++;
        }
    }
    return active;
}

uint32_t ElisaGuestExec_AddressNativeProt(uintptr_t address) {
    return elisa_guest_exec_native_prot_at(address, sizeof(uintptr_t));
}

uintptr_t ElisaGuestExec_ReadNativeU64(uintptr_t address) {
    return elisa_guest_exec_read_native_word(address);
}

void ElisaGuestExec_ResetCrashState(void) {
    elisa_guest_exec_last_status = 0;
    elisa_guest_exec_last_signal = 0;
    elisa_guest_exec_last_errno = 0;
    elisa_guest_exec_last_native_phase = ELISA_GUEST_EXEC_PHASE_NONE;
    elisa_guest_exec_last_fault_address = 0;
    elisa_guest_exec_last_guest_pc = 0;
    elisa_guest_exec_last_guest_sp = 0;
    elisa_guest_exec_last_guest_bp = 0;
    elisa_guest_exec_last_guest_rdi = 0;
    elisa_guest_exec_last_guest_rsi = 0;
    elisa_guest_exec_last_guest_rdx = 0;
    elisa_guest_exec_last_guest_rcx = 0;
    elisa_guest_exec_last_guest_r8 = 0;
    elisa_guest_exec_last_guest_r9 = 0;
    elisa_guest_exec_last_guest_rax = 0;
    elisa_guest_exec_last_guest_rbx = 0;
    elisa_guest_exec_expected_entry_params = 0;
    elisa_guest_exec_expected_stack_word0 = 0;
    elisa_guest_exec_expected_stack_word1 = 0;
    elisa_guest_exec_entry_stack_word0 = 0;
    elisa_guest_exec_entry_stack_word1 = 0;
    elisa_guest_exec_entry_native_word0 = 0;
    elisa_guest_exec_entry_native_word1 = 0;
    elisa_guest_exec_prejump_rdi = 0;
    elisa_guest_exec_prejump_rsi = 0;
    elisa_guest_exec_prejump_rsp = 0;
    elisa_guest_exec_signal_guest_pc = 0;
    elisa_guest_exec_fallback_guest_pc = 0;
    elisa_guest_exec_signal_pc_valid = 0;
    elisa_guest_exec_last_pc_source = ELISA_GUEST_EXEC_PC_SOURCE_NONE;
    elisa_guest_exec_fault_word0 = 0;
    elisa_guest_exec_fault_word1 = 0;
    elisa_guest_exec_signal_stack_word0 = 0;
    elisa_guest_exec_signal_stack_word1 = 0;
    elisa_guest_exec_signal_stack_symbol0 = "";
    elisa_guest_exec_signal_stack_image0 = "";
    elisa_guest_exec_signal_pc_symbol = "";
    elisa_guest_exec_signal_pc_image = "";
    elisa_guest_exec_entry_native_prot = 0xffffffffu;
    elisa_guest_exec_pc_native_prot = 0xffffffffu;
    elisa_guest_exec_last_pc_was_fallback = 0;
    elisa_guest_exec_boundary_callback_count = 0;
    elisa_guest_exec_boundary_callback_reason = 0;
    elisa_guest_exec_failed_relocation_count = 0;
    elisa_guest_exec_synthetic_observed_argc = 0;
    elisa_guest_exec_synthetic_observed_argv0 = 0;
    elisa_guest_exec_synthetic_observed_entry = 0;
    elisa_guest_exec_synthetic_observed_params = 0;
    elisa_guest_exec_synthetic_observed_exit_func = 0;
    elisa_guest_exec_synthetic_observed_stack = 0;
    elisa_guest_exec_aerolib_fallback_caller_pc = 0;
    elisa_guest_exec_aerolib_fallback_slot_va = 0;
    elisa_guest_exec_aerolib_fallback_callsite_bytes = 0;
    elisa_guest_exec_aerolib_fallback_invocations = 0;
    elisa_guest_exec_aerolib_fallback_plt_va = 0;
    elisa_guest_exec_aerolib_fallback_plt_bytes = 0;
    elisa_guest_exec_runtime_hle_module = "";
    elisa_guest_exec_runtime_hle_symbol = "";
    elisa_guest_exec_runtime_hle_arg0 = 0;
    elisa_guest_exec_runtime_hle_arg1 = 0;
    elisa_guest_exec_runtime_hle_arg2 = 0;
    elisa_guest_exec_runtime_hle_ret = 0;
#if !defined(_WIN32)
    elisa_guest_exec_timeout_fired = 0;
#endif
}

int32_t ElisaGuestExec_MapRegion(uintptr_t address, uint64_t size, uint32_t prot) {
    if (address == 0 || size == 0) {
        return -1;
    }
    elisa_guest_exec_clear_writable_page_cache();
#if defined(_WIN32)
    DWORD protect = PAGE_NOACCESS;
    if ((prot & 4u) != 0u) {
        protect = ((prot & 2u) != 0u) ? PAGE_EXECUTE_READWRITE : PAGE_EXECUTE_READ;
    } else if ((prot & 2u) != 0u) {
        protect = PAGE_READWRITE;
    } else if ((prot & 1u) != 0u) {
        protect = PAGE_READONLY;
    }
    void* result = VirtualAlloc((void*)address, (SIZE_T)size, MEM_RESERVE | MEM_COMMIT, protect);
    if (result == (void*)address) {
        elisa_guest_exec_record_native_region(address, size, prot);
        return 0;
    }
    return -1;
#else
    int native_prot = PROT_NONE;
    if ((prot & 1u) != 0u) native_prot |= PROT_READ;
    if ((prot & 2u) != 0u) native_prot |= PROT_WRITE;
    if ((prot & 4u) != 0u) native_prot |= PROT_EXEC;
#if defined(MAP_FIXED_NOREPLACE)
    int flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE;
#else
    int flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED;
#endif
    void* result = mmap((void*)address, (size_t)size, native_prot, flags, -1, 0);
    if (result == MAP_FAILED) {
#if defined(MAP_FIXED_NOREPLACE)
        if (errno == EEXIST) {
            /*
             * Module images are reserved first and then PT_LOAD slices are
             * committed inside that reservation. MAP_FIXED_NOREPLACE reports
             * that as EEXIST; promote the existing subrange instead of leaving
             * it PROT_NONE.
             */
            if (mprotect((void*)address, (size_t)size, native_prot) == 0) {
                elisa_guest_exec_record_native_region(address, size, prot);
                return 0;
            }
            return -1;
        }
#endif
        return -1;
    }
    if (result == (void*)address) {
        elisa_guest_exec_record_native_region(address, size, prot);
        return 0;
    }
    return -1;
#endif
}

int32_t ElisaGuestExec_ProtectRegion(uintptr_t address, uint64_t size, uint32_t prot) {
    if (address == 0 || size == 0) {
        return -1;
    }
    elisa_guest_exec_clear_writable_page_cache();
#if defined(_WIN32)
    DWORD protect = PAGE_NOACCESS;
    DWORD old_protect = 0;
    if ((prot & 4u) != 0u) {
        protect = ((prot & 2u) != 0u) ? PAGE_EXECUTE_READWRITE : PAGE_EXECUTE_READ;
    } else if ((prot & 2u) != 0u) {
        protect = PAGE_READWRITE;
    } else if ((prot & 1u) != 0u) {
        protect = PAGE_READONLY;
    }
    if (VirtualProtect((void*)address, (SIZE_T)size, protect, &old_protect)) {
        elisa_guest_exec_record_native_region(address, size, prot);
        return 0;
    }
    return -1;
#else
    int native_prot = PROT_NONE;
    if ((prot & 1u) != 0u) native_prot |= PROT_READ;
    if ((prot & 2u) != 0u) native_prot |= PROT_WRITE;
    if ((prot & 4u) != 0u) native_prot |= PROT_EXEC;
    if (mprotect((void*)address, (size_t)size, native_prot) == 0) {
        elisa_guest_exec_record_native_region(address, size, prot);
        return 0;
    }
    return -1;
#endif
}

int32_t ElisaGuestExec_UnmapRegion(uintptr_t address, uint64_t size) {
    if (address == 0 || size == 0) {
        return -1;
    }
#if defined(_WIN32)
    (void)size;
    if (VirtualFree((void*)address, 0, MEM_RELEASE)) {
        elisa_guest_exec_forget_native_region(address, size);
        return 0;
    }
    return -1;
#else
    if (munmap((void*)address, (size_t)size) == 0) {
        elisa_guest_exec_forget_native_region(address, size);
        return 0;
    }
    return -1;
#endif
}

uintptr_t ElisaGuestExec_AllocateRegion(uint64_t size, uint32_t prot) {
    if (size == 0) {
        return 0;
    }
#if defined(_WIN32)
    DWORD protect = PAGE_NOACCESS;
    if ((prot & 4u) != 0u) {
        protect = ((prot & 2u) != 0u) ? PAGE_EXECUTE_READWRITE : PAGE_EXECUTE_READ;
    } else if ((prot & 2u) != 0u) {
        protect = PAGE_READWRITE;
    } else if ((prot & 1u) != 0u) {
        protect = PAGE_READONLY;
    }
    void* result = VirtualAlloc(NULL, (SIZE_T)size, MEM_RESERVE | MEM_COMMIT, protect);
    if (result != NULL) {
        elisa_guest_exec_record_native_region((uintptr_t)result, size, prot);
    }
    return (uintptr_t)result;
#else
    int native_prot = PROT_NONE;
    if ((prot & 1u) != 0u) native_prot |= PROT_READ;
    if ((prot & 2u) != 0u) native_prot |= PROT_WRITE;
    if ((prot & 4u) != 0u) native_prot |= PROT_EXEC;
    void* result = mmap(NULL, (size_t)size, native_prot, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (result == MAP_FAILED) {
        return 0;
    }
    elisa_guest_exec_record_native_region((uintptr_t)result, size, prot);
    return (uintptr_t)result;
#endif
}

static uintptr_t elisa_guest_exec_page_size(void) {
#if defined(_WIN32)
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return (uintptr_t)info.dwPageSize;
#else
    long raw_page_size = sysconf(_SC_PAGESIZE);
    return raw_page_size > 0 ? (uintptr_t)raw_page_size : 4096u;
#endif
}

static int32_t elisa_guest_exec_page_cached_writable(uintptr_t page) {
    uint32_t slot = (uint32_t)((page >> 12u) % ELISA_GUEST_EXEC_WRITABLE_PAGE_CACHE);
    return elisa_guest_exec_writable_pages[slot] == page;
}

static void elisa_guest_exec_cache_writable_page(uintptr_t page) {
    uint32_t slot = (uint32_t)((page >> 12u) % ELISA_GUEST_EXEC_WRITABLE_PAGE_CACHE);
    if (elisa_guest_exec_writable_pages[slot] == 0) {
        elisa_guest_exec_writable_page_count++;
    }
    elisa_guest_exec_writable_pages[slot] = page;
}

static int32_t elisa_guest_exec_make_writable_cached(uintptr_t address, uint64_t size) {
    if (address == 0 || size == 0) {
        return 0;
    }
    uintptr_t page_size = elisa_guest_exec_page_size();
    uintptr_t start = address & ~(page_size - 1u);
    uintptr_t end = (address + (uintptr_t)size + page_size - 1u) & ~(page_size - 1u);
    for (uintptr_t page = start; page < end; page += page_size) {
        if (elisa_guest_exec_page_cached_writable(page)) {
            continue;
        }
#if defined(_WIN32)
        DWORD old_protect = 0;
        if (!VirtualProtect((void*)page, (SIZE_T)page_size, PAGE_EXECUTE_READWRITE, &old_protect)) {
            return -1;
        }
#else
        if (mprotect((void*)page, (size_t)page_size, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
            return -1;
        }
#endif
        elisa_guest_exec_cache_writable_page(page);
    }
    return 0;
}

int32_t ElisaGuestExec_WriteBytes(uintptr_t address, const void* data, uint64_t size) {
    if (address == 0 || (data == NULL && size != 0)) {
        return -1;
    }
    if (size != 0) {
        if (elisa_guest_exec_make_writable_cached(address, size) != 0) {
            return -1;
        }
        memcpy((void*)address, data, (size_t)size);
        elisa_guest_exec_flush_instruction_cache(address, size);
    }
    return 0;
}

int32_t ElisaGuestExec_WriteU64(uintptr_t address, uint64_t value) {
    if (address == 0) {
        return -1;
    }
    if (elisa_guest_exec_make_writable_cached(address, sizeof(value)) != 0) {
        return -1;
    }
    memcpy((void*)address, &value, sizeof(value));
    elisa_guest_exec_flush_instruction_cache(address, sizeof(value));
    return 0;
}

uint64_t ElisaGuestExec_RunSyntheticFunction(uintptr_t entry_addr, uint64_t arg0, uint64_t arg1) {
    if (entry_addr == 0) {
        elisa_guest_exec_last_status = -1;
        return UINT64_MAX;
    }
#if defined(__x86_64__) || defined(_M_X64)
    ElisaGuestExec_ResetCrashState();
#if defined(_WIN32)
    ElisaGuestExecTestFunc fn = (ElisaGuestExecTestFunc)entry_addr;
    uint64_t result = fn(arg0, arg1);
    elisa_guest_exec_last_status = 1;
    return result;
#else
    elisa_guest_exec_install_guards();
    elisa_guest_exec_guard_active = 1;
    if (sigsetjmp(elisa_guest_exec_jmp, 1) != 0) {
        elisa_guest_exec_guard_active = 0;
        elisa_guest_exec_restore_guards();
        return UINT64_MAX - 2u;
    }
    ElisaGuestExecTestFunc fn = (ElisaGuestExecTestFunc)entry_addr;
    uint64_t result = fn(arg0, arg1);
    elisa_guest_exec_guard_active = 0;
    elisa_guest_exec_restore_guards();
    elisa_guest_exec_last_status = 1;
    return result;
#endif
#else
    (void)arg0;
    (void)arg1;
    elisa_guest_exec_last_status = -2;
    return UINT64_MAX - 1u;
#endif
}

uint64_t ElisaGuestExec_RunSyntheticFunctionWithTimeout(uintptr_t entry_addr, uint64_t arg0,
                                                        uint64_t arg1, uint32_t timeout_ms) {
    if (entry_addr == 0) {
        elisa_guest_exec_last_status = -1;
        return UINT64_MAX;
    }
#if defined(__x86_64__) && !defined(_WIN32)
    ElisaGuestExec_ResetCrashState();
    elisa_guest_exec_install_guards();
    elisa_guest_exec_guard_active = 1;
    elisa_guest_exec_guard_target_thread = pthread_self();
    elisa_guest_exec_start_watchdog(timeout_ms);
    if (timeout_ms != 0) {
        struct itimerval timer;
        memset(&timer, 0, sizeof(timer));
        timer.it_value.tv_sec = timeout_ms / 1000u;
        timer.it_value.tv_usec = (timeout_ms % 1000u) * 1000u;
        setitimer(ITIMER_REAL, &timer, NULL);
    }
    if (sigsetjmp(elisa_guest_exec_jmp, 1) != 0) {
        struct itimerval timer_off;
        memset(&timer_off, 0, sizeof(timer_off));
        setitimer(ITIMER_REAL, &timer_off, NULL);
        elisa_guest_exec_guard_active = 0;
        elisa_guest_exec_restore_guards();
        if (elisa_guest_exec_timeout_fired) {
            elisa_guest_exec_last_status = -3;
            return UINT64_MAX - 3u;
        }
        return UINT64_MAX - 2u;
    }
    ElisaGuestExecTestFunc fn = (ElisaGuestExecTestFunc)entry_addr;
    uint64_t result = fn(arg0, arg1);
    struct itimerval timer_off;
    memset(&timer_off, 0, sizeof(timer_off));
    setitimer(ITIMER_REAL, &timer_off, NULL);
    elisa_guest_exec_guard_active = 0;
    elisa_guest_exec_restore_guards();
    elisa_guest_exec_last_status = 1;
    return result;
#else
    (void)arg0;
    (void)arg1;
    (void)timeout_ms;
    elisa_guest_exec_last_status = -2;
    return UINT64_MAX - 1u;
#endif
}

uint64_t ElisaGuestExec_SyntheticCrash(uint64_t arg0, uint64_t arg1) {
    (void)arg0;
    (void)arg1;
    volatile uint64_t* ptr = (volatile uint64_t*)0;
    return *ptr;
}

uint64_t ElisaGuestExec_SyntheticAdd(uint64_t arg0, uint64_t arg1) {
    return arg0 + arg1 + 0x1234u;
}

uint64_t ElisaGuestExec_SyntheticSpin(uint64_t arg0, uint64_t arg1) {
    volatile uint64_t sink = arg0 ^ arg1;
    for (;;) {
        sink += 1u;
    }
}

void ElisaGuestExec_SyntheticMainReturn(ElisaGuestEntryParams* params, ElisaGuestExitFunc exit_func) {
    elisa_guest_exec_synthetic_observed_stack = elisa_guest_exec_current_stack_pointer();
    elisa_guest_exec_synthetic_observed_params = (uintptr_t)params;
    elisa_guest_exec_synthetic_observed_exit_func = (uintptr_t)exit_func;
    if (params != NULL) {
        elisa_guest_exec_synthetic_observed_argc = params->argc;
        elisa_guest_exec_synthetic_observed_argv0 = (uintptr_t)params->argv[0];
        elisa_guest_exec_synthetic_observed_entry = params->entry_addr;
    }
    if (params == NULL || params->argc != 77) {
        elisa_guest_exec_last_status = -4;
        return;
    }
    elisa_guest_exec_last_status = 1;
}

void ElisaGuestExec_SyntheticBoundaryCallback(uint64_t reason) {
    elisa_guest_exec_boundary_callback_count++;
    elisa_guest_exec_boundary_callback_reason = reason;
    elisa_guest_exec_last_status = 2;
}

void ElisaGuestExec_SyntheticMainCallBoundary(ElisaGuestEntryParams* params,
                                              ElisaGuestExitFunc exit_func) {
    (void)exit_func;
    if (params == NULL || params->argc != 88 || params->argv[0] == NULL) {
        elisa_guest_exec_last_status = -5;
        return;
    }
    ElisaGuestExecBoundaryFunc boundary = (ElisaGuestExecBoundaryFunc)(uintptr_t)params->argv[0];
    boundary(0xB000000000000042ull);
}

void ElisaGuestExec_NonSyntheticMainReportBoundary(ElisaGuestEntryParams* params,
                                                   ElisaGuestExitFunc exit_func) {
    (void)params;
    (void)exit_func;
    ElisaGuestExec_ReportFirstBoundary(1);
}

uintptr_t ElisaGuestExec_GetSyntheticAddAddress(void) {
    return (uintptr_t)&ElisaGuestExec_SyntheticAdd;
}

uintptr_t ElisaGuestExec_GetSyntheticCrashAddress(void) {
    return (uintptr_t)&ElisaGuestExec_SyntheticCrash;
}

uintptr_t ElisaGuestExec_GetSyntheticSpinAddress(void) {
    return (uintptr_t)&ElisaGuestExec_SyntheticSpin;
}

uintptr_t ElisaGuestExec_GetSyntheticMainReturnAddress(void) {
    return (uintptr_t)&ElisaGuestExec_SyntheticMainReturn;
}

uintptr_t ElisaGuestExec_GetSyntheticMainCallBoundaryAddress(void) {
    return (uintptr_t)&ElisaGuestExec_SyntheticMainCallBoundary;
}

uintptr_t ElisaGuestExec_GetSyntheticBoundaryCallbackAddress(void) {
    return (uintptr_t)&ElisaGuestExec_SyntheticBoundaryCallback;
}

uintptr_t ElisaGuestExec_GetNonSyntheticMainReportBoundaryAddress(void) {
    return (uintptr_t)&ElisaGuestExec_NonSyntheticMainReportBoundary;
}

static int32_t __attribute__((unused)) elisa_guest_exec_is_synthetic_main_entry(uintptr_t entry_addr) {
    return entry_addr == (uintptr_t)&ElisaGuestExec_SyntheticMainReturn ||
           entry_addr == (uintptr_t)&ElisaGuestExec_SyntheticMainCallBoundary;
}

int32_t ElisaGuestExec_BoundaryCallbackCount(void) {
    return elisa_guest_exec_boundary_callback_count;
}

uint64_t ElisaGuestExec_BoundaryCallbackReason(void) {
    return elisa_guest_exec_boundary_callback_reason;
}

int32_t ElisaGuestExec_SyntheticObservedArgc(void) {
    return elisa_guest_exec_synthetic_observed_argc;
}

uintptr_t ElisaGuestExec_SyntheticObservedArgv0(void) {
    return elisa_guest_exec_synthetic_observed_argv0;
}

uintptr_t ElisaGuestExec_SyntheticObservedEntry(void) {
    return elisa_guest_exec_synthetic_observed_entry;
}

uintptr_t ElisaGuestExec_SyntheticObservedParams(void) {
    return elisa_guest_exec_synthetic_observed_params;
}

uintptr_t ElisaGuestExec_SyntheticObservedExitFunc(void) {
    return elisa_guest_exec_synthetic_observed_exit_func;
}

uintptr_t ElisaGuestExec_SyntheticObservedStack(void) {
    return elisa_guest_exec_synthetic_observed_stack;
}

uint64_t ElisaGuestExec_SyntheticObservedStackMod16(void) {
    return (uint64_t)(elisa_guest_exec_synthetic_observed_stack & 0xFull);
}

#if defined(__x86_64__) || defined(_M_X64)
#if defined(_WIN32)
int32_t ElisaGuestExec_RunMainEntry(ElisaGuestEntryParams* params, ElisaGuestExitFunc exit_func) {
    (void)params;
    (void)exit_func;
    return -2;
}
#else
__attribute__((noreturn)) void ElisaGuestExec_RunMainEntry(ElisaGuestEntryParams* params,
                                                           ElisaGuestExitFunc exit_func) {
    if (exit_func == NULL) {
        exit_func = elisa_guest_exec_default_exit;
    }
    elisa_guest_exec_enter_main_entry_asm(params, exit_func);
}
#endif
#else
int32_t ElisaGuestExec_RunMainEntry(ElisaGuestEntryParams* params, ElisaGuestExitFunc exit_func) {
    (void)params;
    (void)exit_func;
    return -2;
}
#endif

int32_t ElisaGuestExec_RunMainEntryGuarded(ElisaGuestEntryParams* params, uint32_t timeout_ms) {
    elisa_guest_exec_set_native_phase(ELISA_GUEST_EXEC_PHASE_FUNCTION_ENTRY);
    if (params == NULL || params->entry_addr == 0) {
        ElisaGuestExec_ResetCrashState();
        elisa_guest_exec_set_native_phase(ELISA_GUEST_EXEC_PHASE_RETURN_BAD_PARAMS);
        elisa_guest_exec_last_status = -1;
        return -1;
    }
#if defined(__x86_64__) && !defined(_WIN32)
    ElisaGuestExec_ResetCrashState();
    elisa_guest_exec_set_native_phase(ELISA_GUEST_EXEC_PHASE_AFTER_RESET);
    elisa_guest_exec_last_errno = -100;
    if (elisa_guest_exec_is_synthetic_main_entry(params->entry_addr)) {
        elisa_guest_exec_set_native_phase(ELISA_GUEST_EXEC_PHASE_SYNTHETIC_ENTRY);
        elisa_guest_exec_install_guards();
        elisa_guest_exec_guard_active = 1;
        elisa_guest_exec_guard_target_thread = pthread_self();
        elisa_guest_exec_start_watchdog(timeout_ms);
        if (timeout_ms != 0) {
            struct itimerval timer;
            memset(&timer, 0, sizeof(timer));
            timer.it_value.tv_sec = timeout_ms / 1000u;
            timer.it_value.tv_usec = (timeout_ms % 1000u) * 1000u;
            setitimer(ITIMER_REAL, &timer, NULL);
        }
        elisa_guest_exec_set_native_phase(ELISA_GUEST_EXEC_PHASE_SYNTHETIC_BEFORE_SIGSETJMP);
        if (sigsetjmp(elisa_guest_exec_jmp, 1) != 0) {
            struct itimerval timer_off;
            memset(&timer_off, 0, sizeof(timer_off));
            setitimer(ITIMER_REAL, &timer_off, NULL);
            elisa_guest_exec_guard_active = 0;
            elisa_guest_exec_restore_guards();
            if (elisa_guest_exec_timeout_fired) {
                elisa_guest_exec_last_status = -3;
            }
            return elisa_guest_exec_last_status;
        }
        elisa_guest_exec_set_native_phase(ELISA_GUEST_EXEC_PHASE_SYNTHETIC_BEFORE_MAIN_ENTRY);
        elisa_guest_exec_call_main_entry(params, elisa_guest_exec_guarded_exit);
        struct itimerval timer_off;
        memset(&timer_off, 0, sizeof(timer_off));
        setitimer(ITIMER_REAL, &timer_off, NULL);
        elisa_guest_exec_guard_active = 0;
        elisa_guest_exec_restore_guards();
        elisa_guest_exec_last_status = 1;
        return 1;
    }
    elisa_guest_exec_set_native_phase(ELISA_GUEST_EXEC_PHASE_BEFORE_FORK_REPORT_SETUP);
    if (elisa_guest_exec_ensure_fork_report() != 0) {
        elisa_guest_exec_set_native_phase(ELISA_GUEST_EXEC_PHASE_RETURN_FORK_REPORT_SETUP_FAILED);
        elisa_guest_exec_last_errno = errno != 0 ? errno : -101;
        elisa_guest_exec_last_status = -1;
        return -1;
    }
    elisa_guest_exec_set_native_phase(ELISA_GUEST_EXEC_PHASE_AFTER_FORK_REPORT_SETUP);
    elisa_guest_exec_last_errno = -102;
    elisa_guest_exec_set_native_phase(ELISA_GUEST_EXEC_PHASE_BEFORE_FORK);
    pid_t child = fork();
    if (child < 0) {
        elisa_guest_exec_set_native_phase(ELISA_GUEST_EXEC_PHASE_RETURN_FORK_FAILED);
        elisa_guest_exec_last_errno = errno != 0 ? errno : -103;
        elisa_guest_exec_last_status = -1;
        return -1;
    }
    if (child == 0) {
        elisa_guest_exec_run_child_main_entry(params);
    }
    elisa_guest_exec_set_native_phase(ELISA_GUEST_EXEC_PHASE_PARENT_BRANCH_ENTERED);
    uint32_t waited_ms = 0;
    uint32_t sleep_step_ms = 1;
    int status = 0;
    for (;;) {
        pid_t result = waitpid(child, &status, WNOHANG);
        if (result == child) {
            elisa_guest_exec_set_native_phase(ELISA_GUEST_EXEC_PHASE_PARENT_WAITPID_CHILD);
            elisa_guest_exec_last_errno = -104;
            if (WIFSIGNALED(status)) {
                elisa_guest_exec_set_native_phase(ELISA_GUEST_EXEC_PHASE_PARENT_WIFSIGNALED);
                int sig = WTERMSIG(status);
                if (elisa_guest_exec_fork_report != NULL) {
                    if (elisa_guest_exec_fork_report->status == 0) {
                        elisa_guest_exec_fork_report->status = -sig;
                    }
                    if (elisa_guest_exec_fork_report->signal == 0) {
                        elisa_guest_exec_fork_report->signal = sig;
                    }
                }
                elisa_guest_exec_absorb_fork_report();
                if (elisa_guest_exec_last_status == 0) {
                    elisa_guest_exec_last_status = -sig;
                }
                if (elisa_guest_exec_last_signal == 0) {
                    elisa_guest_exec_last_signal = sig;
                }
                return elisa_guest_exec_last_status;
            }
            if (WIFEXITED(status)) {
                elisa_guest_exec_set_native_phase(ELISA_GUEST_EXEC_PHASE_PARENT_WIFEXITED);
                int code = WEXITSTATUS(status);
                if (elisa_guest_exec_fork_report != NULL) {
                    if (elisa_guest_exec_fork_report->status == 0) {
                        elisa_guest_exec_fork_report->status = code == 0 ? -11 :
                            (code >= 128 ? -(code - 128) : -code);
                    }
                    if (code >= 128 && elisa_guest_exec_fork_report->signal == 0) {
                        elisa_guest_exec_fork_report->signal = code - 128;
                    }
                }
                elisa_guest_exec_absorb_fork_report();
                if (elisa_guest_exec_last_status == 0) {
                    elisa_guest_exec_last_status = code == 0 ? -11 : -code;
                }
                return elisa_guest_exec_last_status;
            }
            if (WIFSTOPPED(status) || WIFCONTINUED(status)) {
                elisa_guest_exec_set_native_phase(ELISA_GUEST_EXEC_PHASE_PARENT_WAIT_STATUS_OTHER);
                continue;
            }
            elisa_guest_exec_set_native_phase(ELISA_GUEST_EXEC_PHASE_PARENT_WAIT_STATUS_OTHER);
            elisa_guest_exec_last_errno = status;
            elisa_guest_exec_absorb_fork_report();
            if (elisa_guest_exec_last_status == 0) {
                elisa_guest_exec_last_status = -1;
            }
            return elisa_guest_exec_last_status;
        }
        if (result < 0) {
            if (errno == EINTR) {
                elisa_guest_exec_set_native_phase(ELISA_GUEST_EXEC_PHASE_PARENT_WAITPID_EINTR);
                continue;
            }
            elisa_guest_exec_set_native_phase(ELISA_GUEST_EXEC_PHASE_PARENT_WAITPID_ERROR);
            elisa_guest_exec_last_errno = errno != 0 ? errno : -105;
            elisa_guest_exec_absorb_fork_report();
            if (elisa_guest_exec_last_status == 0) {
                elisa_guest_exec_last_status = -1;
            }
            return elisa_guest_exec_last_status;
        }
        if (timeout_ms != 0 && waited_ms >= timeout_ms) {
            elisa_guest_exec_set_native_phase(ELISA_GUEST_EXEC_PHASE_PARENT_TIMEOUT);
            kill(child, SIGKILL);
            (void)waitpid(child, &status, 0);
            elisa_guest_exec_timeout_fired = 1;
            elisa_guest_exec_last_status = -3;
            elisa_guest_exec_last_signal = SIGALRM;
            if (elisa_guest_exec_fork_report != NULL) {
                elisa_guest_exec_fork_report->status = -3;
                elisa_guest_exec_fork_report->signal = SIGALRM;
            }
            elisa_guest_exec_absorb_fork_report();
            return -3;
        }
        usleep((useconds_t)sleep_step_ms * 1000u);
        waited_ms += sleep_step_ms;
    }
#else
    (void)timeout_ms;
    ElisaGuestExec_ResetCrashState();
    elisa_guest_exec_set_native_phase(ELISA_GUEST_EXEC_PHASE_UNSUPPORTED_RETURN);
    elisa_guest_exec_last_status = -2;
    return -2;
#endif
}

void ElisaGuestExec_EmitCusaArtifactStage(void) {
    fprintf(stdout, "CUSA07399_EXEC_STAGE stage=load link=pass handoff=pass\n");
    fflush(stdout);
}

void ElisaGuestExec_EmitCusaArtifactSummary(uint64_t module_count, uint64_t total_imports,
                                            uint64_t unresolved_imports,
                                            uint64_t relocated_imports,
                                            uint64_t native_hle_resolved,
                                            uint64_t prx_export_resolved,
                                            uint64_t aerolib_fallback_resolved,
                                            uint64_t hle_symbols, uint64_t entry,
                                            uint64_t main_entry, int32_t guarded_status,
                                            int32_t last_signal, uint64_t fault_address,
                                            uint64_t last_guest_pc,
                                            int32_t boundary_status,
                                            int32_t boundary_reason,
                                            int32_t execution_stage) {
    fprintf(stdout,
            "CUSA07399_ARTIFACT module_count=%llu total_imports=%llu "
            "unresolved_imports=%llu relocated_imports=%llu native_hle=%llu "
            "prx_export=%llu aerolib_fallback=%llu hle_symbols=%llu\n",
            (unsigned long long)module_count, (unsigned long long)total_imports,
            (unsigned long long)unresolved_imports,
            (unsigned long long)relocated_imports,
            (unsigned long long)native_hle_resolved,
            (unsigned long long)prx_export_resolved,
            (unsigned long long)aerolib_fallback_resolved,
            (unsigned long long)hle_symbols);
    fprintf(stdout,
            "CUSA07399_ARTIFACT entry=0x%llx main_entry=0x%llx guarded_status=%d "
            "last_signal=%d fault=0x%llx last_guest_pc=0x%llx\n",
            (unsigned long long)entry, (unsigned long long)main_entry, guarded_status,
            last_signal, (unsigned long long)fault_address, (unsigned long long)last_guest_pc);
    fprintf(stdout,
            "CUSA07399_ARTIFACT execution_stage=%d boundary_status=%d boundary_reason=%d\n",
            execution_stage, boundary_status, boundary_reason);
    fprintf(stdout,
            "CUSA07399_ARTIFACT failed_relocations=%llu\n",
            (unsigned long long)elisa_guest_exec_failed_relocation_count);
    fflush(stdout);
}

void ElisaGuestExec_EmitCusaArtifactModule(const char* module, const char* host, int32_t shared,
                                           uint64_t relocations, uint64_t imports,
                                           uint64_t resolved, uint64_t native_hle,
                                           uint64_t prx_export, uint64_t aerolib_fallback,
                                           uint64_t malformed, uint64_t unresolved) {
    fprintf(stdout,
            "CUSA07399_ARTIFACT module=%s host=%s shared=%d relocations=%llu "
            "imports=%llu resolved=%llu native_hle=%llu prx_export=%llu "
            "aerolib_fallback=%llu malformed=%llu unresolved=%llu\n",
            module != NULL ? module : "", host != NULL ? host : "", shared,
            (unsigned long long)relocations, (unsigned long long)imports,
            (unsigned long long)resolved, (unsigned long long)native_hle,
            (unsigned long long)prx_export, (unsigned long long)aerolib_fallback,
            (unsigned long long)malformed, (unsigned long long)unresolved);
    fflush(stdout);
}

void ElisaGuestExec_EmitCusaArtifactImport(const char* source, const char* nid,
                                           const char* library, const char* module,
                                           const char* subsystem,
                                           uint64_t resolution) {
    fprintf(stdout,
            "CUSA07399_ARTIFACT fallback_import source=%s nid=%s library=%s "
            "module=%s subsystem=%s resolution=%llu\n",
            source != NULL ? source : "", nid != NULL ? nid : "",
            library != NULL ? library : "", module != NULL ? module : "",
            subsystem != NULL ? subsystem : "",
            (unsigned long long)resolution);
    fflush(stdout);
}

void ElisaGuestExec_EmitCusaArtifactLastHle(const char* module, const char* symbol) {
    fprintf(stdout, "CUSA07399_ARTIFACT last_hle_module=%s last_hle_symbol=%s\n",
            module != NULL ? module : "", symbol != NULL ? symbol : "");
    fflush(stdout);
}

void ElisaGuestExec_EmitCusaArtifactStringKV(const char* key, const char* value) {
    fprintf(stdout, "CUSA07399_ARTIFACT %s=%s\n",
            key != NULL ? key : "key",
            value != NULL ? value : "");
    fflush(stdout);
}

void ElisaGuestExec_EmitCusaArtifactKV(const char* key, uint64_t value) {
    fprintf(stdout, "CUSA07399_ARTIFACT %s=%llu\n",
            key != NULL ? key : "key",
            (unsigned long long)value);
    fflush(stdout);
}

void ElisaGuestExec_SetFailedRelocationCount(uint64_t count) {
    elisa_guest_exec_failed_relocation_count = count;
}

#if defined(ELISA_GUEST_EXEC_STANDALONE_TEST)
static int elisa_guest_exec_expect_u64(const char* name, uint64_t got, uint64_t want) {
    if (got != want) {
        fprintf(stderr, "%s: got=%llu want=%llu\n", name,
                (unsigned long long)got, (unsigned long long)want);
        return 1;
    }
    return 0;
}

static int elisa_guest_exec_expect_s32(const char* name, int32_t got, int32_t want) {
    if (got != want) {
        fprintf(stderr, "%s: got=%d want=%d\n", name, got, want);
        return 1;
    }
    return 0;
}

int main(void) {
    int failed = 0;
    if (ElisaGuestExec_IsSupported() != 1) {
        fprintf(stderr, "guest exec runtime is not supported in standalone x64 helper\n");
        return 2;
    }
    failed |= elisa_guest_exec_expect_u64(
        "synthetic-add",
        ElisaGuestExec_RunSyntheticFunction(ElisaGuestExec_GetSyntheticAddAddress(), 40u, 2u),
        0x125Eu);
    failed |= elisa_guest_exec_expect_s32("synthetic-add-status",
                                          ElisaGuestExec_LastStatus(), 1);

    failed |= elisa_guest_exec_expect_u64(
        "synthetic-timeout",
        ElisaGuestExec_RunSyntheticFunctionWithTimeout(
            ElisaGuestExec_GetSyntheticSpinAddress(), 1u, 2u, 10u),
        UINT64_MAX - 3u);
    failed |= elisa_guest_exec_expect_s32("synthetic-timeout-status",
                                          ElisaGuestExec_LastStatus(), -3);

    ElisaGuestEntryParams params;
    memset(&params, 0, sizeof(params));
    params.argc = 77;
    params.argv[0] = (const char*)0x12345678ull;
    params.entry_addr = ElisaGuestExec_GetSyntheticMainReturnAddress();
    int32_t guarded_status = ElisaGuestExec_RunMainEntryGuarded(&params, 100u);
    failed |= elisa_guest_exec_expect_s32("guarded-main-status", guarded_status, 1);
    failed |= elisa_guest_exec_expect_s32("guarded-main-last-status",
                                          ElisaGuestExec_LastStatus(), 1);
    failed |= elisa_guest_exec_expect_s32("guarded-main-observed-argc",
                                          ElisaGuestExec_SyntheticObservedArgc(), 77);
    failed |= elisa_guest_exec_expect_u64("guarded-main-observed-argv0",
                                          (uint64_t)ElisaGuestExec_SyntheticObservedArgv0(),
                                          0x12345678ull);
    failed |= elisa_guest_exec_expect_u64("guarded-main-observed-entry",
                                          (uint64_t)ElisaGuestExec_SyntheticObservedEntry(),
                                          (uint64_t)params.entry_addr);
    if (ElisaGuestExec_SyntheticObservedParams() == 0 ||
        ElisaGuestExec_SyntheticObservedExitFunc() == 0 ||
        ElisaGuestExec_SyntheticObservedStack() == 0) {
        fprintf(stderr,
                "guarded-main-observed-abi: params=0x%llx exit=0x%llx stack=0x%llx\n",
                (unsigned long long)ElisaGuestExec_SyntheticObservedParams(),
                (unsigned long long)ElisaGuestExec_SyntheticObservedExitFunc(),
                (unsigned long long)ElisaGuestExec_SyntheticObservedStack());
        failed = 1;
    }

    memset(&params, 0, sizeof(params));
    params.argc = 88;
    params.entry_addr = ElisaGuestExec_GetSyntheticMainCallBoundaryAddress();
    params.argv[0] = (const char*)ElisaGuestExec_GetSyntheticBoundaryCallbackAddress();
    guarded_status = ElisaGuestExec_RunMainEntryGuarded(&params, 100u);
    failed |= elisa_guest_exec_expect_s32("guarded-boundary-status", guarded_status, 1);
    failed |= elisa_guest_exec_expect_s32("guarded-boundary-callback-count",
                                          ElisaGuestExec_BoundaryCallbackCount(), 1);
    failed |= elisa_guest_exec_expect_u64("guarded-boundary-callback-reason",
                                          ElisaGuestExec_BoundaryCallbackReason(),
                                          0xB000000000000042ull);

    memset(&params, 0, sizeof(params));
    params.argc = 89;
    params.entry_addr = (uintptr_t)&ElisaGuestExec_NonSyntheticMainReportBoundary;
    guarded_status = ElisaGuestExec_RunMainEntryGuarded(&params, 100u);
    failed |= elisa_guest_exec_expect_s32("guarded-fork-boundary-status",
                                          guarded_status, -11);
    failed |= elisa_guest_exec_expect_s32("guarded-fork-boundary-reached",
                                          ElisaGuestExec_ReportedBoundaryReached(), 1);
    failed |= elisa_guest_exec_expect_s32("guarded-fork-boundary-reason",
                                          ElisaGuestExec_ReportedBoundaryReason(), 1);

    memset(&params, 0, sizeof(params));
    params.argc = 0;
    params.entry_addr = ElisaGuestExec_GetSyntheticCrashAddress();
    guarded_status = ElisaGuestExec_RunMainEntryGuarded(&params, 100u);
    failed |= elisa_guest_exec_expect_s32("guarded-fork-crash-status",
                                          guarded_status, -11);
    if (ElisaGuestExec_LastSignal() == 0 || ElisaGuestExec_LastGuestPC() == 0) {
        fprintf(stderr, "guarded-fork-crash-report: signal=%d pc=0x%llx\n",
                ElisaGuestExec_LastSignal(), (unsigned long long)ElisaGuestExec_LastGuestPC());
        failed = 1;
    }
    if (failed != 0) {
        return 1;
    }
    printf("ELISA_GUEST_EXEC_X64_STANDALONE_OK arch=%s mode=%s\n",
           ElisaGuestExec_HostArchitecture(), ElisaGuestExec_HostMode());
    return 0;
}
#endif
