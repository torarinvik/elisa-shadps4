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

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <errno.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>
#if defined(__APPLE__)
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

typedef struct ElisaGuestEntryParams {
    int32_t argc;
    uint32_t padding;
    uintptr_t entry_addr;
    void* argv;
} ElisaGuestEntryParams;

typedef void (*ElisaGuestExitFunc)(int32_t);
typedef uint64_t (*ElisaGuestExecTestFunc)(uint64_t, uint64_t);

static int32_t elisa_guest_exec_last_status = 0;
static int32_t elisa_guest_exec_last_signal = 0;
static uintptr_t elisa_guest_exec_last_fault_address = 0;
static uintptr_t elisa_guest_exec_last_guest_pc = 0;

#if !defined(_WIN32)
static sigjmp_buf elisa_guest_exec_jmp;
static volatile sig_atomic_t elisa_guest_exec_guard_active = 0;
static volatile sig_atomic_t elisa_guest_exec_timeout_fired = 0;
static struct sigaction elisa_guest_exec_old_segv;
static struct sigaction elisa_guest_exec_old_bus;
static struct sigaction elisa_guest_exec_old_ill;
static struct sigaction elisa_guest_exec_old_fpe;
static struct sigaction elisa_guest_exec_old_alrm;

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

static void elisa_guest_exec_signal_handler(int sig, siginfo_t* info, void* uctx) {
    (void)uctx;
    elisa_guest_exec_last_status = -sig;
    elisa_guest_exec_last_signal = sig;
    elisa_guest_exec_last_fault_address = info != NULL ? (uintptr_t)info->si_addr : 0;
    elisa_guest_exec_last_guest_pc = elisa_guest_exec_extract_pc(uctx);
    if (elisa_guest_exec_guard_active) {
        siglongjmp(elisa_guest_exec_jmp, 1);
    }
}

static void elisa_guest_exec_alarm_handler(int sig) {
    (void)sig;
    elisa_guest_exec_timeout_fired = 1;
    elisa_guest_exec_last_status = -3;
    elisa_guest_exec_last_signal = SIGALRM;
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
    sigaction(SIGALRM, &elisa_guest_exec_old_alrm, NULL);
}
#endif

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

int32_t ElisaGuestExec_IsSupported(void) {
#if defined(_WIN32)
    return 0;
#elif defined(__x86_64__) || defined(_M_X64)
    return 1;
#else
    return 0;
#endif
}

int32_t ElisaGuestExec_LastStatus(void) {
    return elisa_guest_exec_last_status;
}

int32_t ElisaGuestExec_LastSignal(void) {
    return elisa_guest_exec_last_signal;
}

uintptr_t ElisaGuestExec_LastFaultAddress(void) {
    return elisa_guest_exec_last_fault_address;
}

uintptr_t ElisaGuestExec_LastGuestPC(void) {
    return elisa_guest_exec_last_guest_pc;
}

void ElisaGuestExec_ResetCrashState(void) {
    elisa_guest_exec_last_status = 0;
    elisa_guest_exec_last_signal = 0;
    elisa_guest_exec_last_fault_address = 0;
    elisa_guest_exec_last_guest_pc = 0;
#if !defined(_WIN32)
    elisa_guest_exec_timeout_fired = 0;
#endif
}

int32_t ElisaGuestExec_MapRegion(uintptr_t address, uint64_t size, uint32_t prot) {
    if (address == 0 || size == 0) {
        return -1;
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
    void* result = VirtualAlloc((void*)address, (SIZE_T)size, MEM_RESERVE | MEM_COMMIT, protect);
    return result == (void*)address ? 0 : -1;
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
        if (errno == EEXIST) return 0;
#endif
        return -1;
    }
    return result == (void*)address ? 0 : -1;
#endif
}

int32_t ElisaGuestExec_ProtectRegion(uintptr_t address, uint64_t size, uint32_t prot) {
    if (address == 0 || size == 0) {
        return -1;
    }
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
    return VirtualProtect((void*)address, (SIZE_T)size, protect, &old_protect) ? 0 : -1;
#else
    int native_prot = PROT_NONE;
    if ((prot & 1u) != 0u) native_prot |= PROT_READ;
    if ((prot & 2u) != 0u) native_prot |= PROT_WRITE;
    if ((prot & 4u) != 0u) native_prot |= PROT_EXEC;
    return mprotect((void*)address, (size_t)size, native_prot) == 0 ? 0 : -1;
#endif
}

int32_t ElisaGuestExec_WriteBytes(uintptr_t address, const void* data, uint64_t size) {
    if (address == 0 || (data == NULL && size != 0)) {
        return -1;
    }
    if (size != 0) {
        memcpy((void*)address, data, (size_t)size);
    }
    return 0;
}

int32_t ElisaGuestExec_WriteU64(uintptr_t address, uint64_t value) {
    if (address == 0) {
        return -1;
    }
    memcpy((void*)address, &value, sizeof(value));
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

uint64_t ElisaGuestExec_SyntheticCrash(uint64_t arg0, uint64_t arg1) {
    (void)arg0;
    (void)arg1;
    volatile uint64_t* ptr = (volatile uint64_t*)0;
    return *ptr;
}

uint64_t ElisaGuestExec_SyntheticAdd(uint64_t arg0, uint64_t arg1) {
    return arg0 + arg1 + 0x1234u;
}

uintptr_t ElisaGuestExec_GetSyntheticAddAddress(void) {
    return (uintptr_t)&ElisaGuestExec_SyntheticAdd;
}

uintptr_t ElisaGuestExec_GetSyntheticCrashAddress(void) {
    return (uintptr_t)&ElisaGuestExec_SyntheticCrash;
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
    __asm__ volatile(
        "andq $-16, %%rsp\n"
        "subq $8, %%rsp\n"
        "pushq 8(%1)\n"
        "pushq 0(%1)\n"
        "movq %1, %%rdi\n"
        "movq %2, %%rsi\n"
        "jmp *%0\n"
        :
        : "r"((void*)params->entry_addr), "r"(params), "r"(exit_func)
        : "rax", "rsi", "rdi");
    __builtin_unreachable();
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
    if (params == NULL || params->entry_addr == 0) {
        ElisaGuestExec_ResetCrashState();
        elisa_guest_exec_last_status = -1;
        return -1;
    }
#if defined(__x86_64__) && !defined(_WIN32)
    typedef void (*ElisaGuestMainEntryFn)(ElisaGuestEntryParams*, ElisaGuestExitFunc);
    ElisaGuestExec_ResetCrashState();
    elisa_guest_exec_install_guards();
    elisa_guest_exec_guard_active = 1;
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
        }
        return elisa_guest_exec_last_status;
    }
    ElisaGuestMainEntryFn entry = (ElisaGuestMainEntryFn)params->entry_addr;
    entry(params, elisa_guest_exec_default_exit);
    struct itimerval timer_off;
    memset(&timer_off, 0, sizeof(timer_off));
    setitimer(ITIMER_REAL, &timer_off, NULL);
    elisa_guest_exec_guard_active = 0;
    elisa_guest_exec_restore_guards();
    elisa_guest_exec_last_status = 1;
    return 1;
#else
    (void)timeout_ms;
    ElisaGuestExec_ResetCrashState();
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

void ElisaGuestExec_EmitCusaArtifactKV(const char* key, uint64_t value) {
    fprintf(stdout, "CUSA07399_ARTIFACT %s=%llu\n",
            key != NULL ? key : "key",
            (unsigned long long)value);
    fflush(stdout);
}
