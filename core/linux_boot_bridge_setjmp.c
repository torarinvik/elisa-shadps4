/* linux_boot_bridge_setjmp.c
 *
 * Linux/glibc linkage shim for the guarded in-process guest-execution path.
 *
 * On glibc <setjmp.h>, `sigsetjmp` is a MACRO expanding to
 * `__sigsetjmp(env, savesigs)` -- there is no exported symbol named
 * `sigsetjmp`. The Elisa side declares
 *     @link_name(sigsetjmp) extern ge_sigsetjmp(env: void&?, savemask: int) -> int
 * which makes the linker look for a symbol literally named `sigsetjmp`,
 * leaving it undefined (masked today by --unresolved-symbols=ignore-all,
 * resolving to NULL -> segfault when the guarded path runs).
 *
 * Fix: provide a real exported function named `sigsetjmp` that forwards to
 * glibc's real `__sigsetjmp`. We must NOT include <setjmp.h> for the
 * definition (the macro would rewrite our function name); instead declare
 * __sigsetjmp ourselves.
 *
 * `siglongjmp` is a genuine exported glibc symbol (verified via readelf:
 * `siglongjmp@@GLIBC_2.2.5`), so no shim is needed for it -- the
 * @link_name(siglongjmp) extern resolves directly. Only `sigsetjmp` is
 * defined here.
 *
 * jmp_buf size note: the Elisa side allocates `ge_jmp_buf: u8[256]`.
 * glibc sizeof(sigjmp_buf) == 200 bytes (struct __jmp_buf_tag[1]), so the
 * 256-byte Elisa buffer is comfortably large enough. We deliberately take
 * `void *` here so this file has no dependency on the jmp_buf layout.
 */

extern int __sigsetjmp(void *env, int savesigs);

/* Real symbol named exactly `sigsetjmp` (the @link_name target). */
int sigsetjmp(void *env, int savesigs)
{
    return __sigsetjmp(env, savesigs);
}
