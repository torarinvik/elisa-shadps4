#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT="${ROOT}/guest_exec_x86_acceptance_latest.txt"

cd "${ROOT}"
{
  echo "GUEST_EXEC_X86_ACCEPTANCE started=$(date -u '+%Y-%m-%dT%H:%M:%SZ')"
  echo "host_uname=$(uname -a)"
  echo "host_machine=$(uname -m)"

  echo "== C ucontext offset reference =="
  ref_c="$(mktemp /tmp/guest_exec_ucontext_ref.XXXXXX.c)"
  ref_bin="$(mktemp /tmp/guest_exec_ucontext_ref.XXXXXX)"
  cat >"${ref_c}" <<'EOF'
#define _GNU_SOURCE
#include <stddef.h>
#include <stdio.h>
#include <sys/ucontext.h>
#include <ucontext.h>
int main(void) {
#if defined(__APPLE__) && defined(__x86_64__)
  printf("platform=apple-x86_64 uc_mcontext=%zu ss=%zu rip=%zu rsp=%zu rbp=%zu rdi=%zu rsi=%zu\n",
         offsetof(ucontext_t, uc_mcontext),
         offsetof(struct __darwin_mcontext64, __ss),
         offsetof(struct __darwin_x86_thread_state64, __rip),
         offsetof(struct __darwin_x86_thread_state64, __rsp),
         offsetof(struct __darwin_x86_thread_state64, __rbp),
         offsetof(struct __darwin_x86_thread_state64, __rdi),
         offsetof(struct __darwin_x86_thread_state64, __rsi));
#elif defined(__linux__) && defined(__x86_64__)
  printf("platform=linux-x86_64 uc_mcontext=%zu REG_RIP=%d REG_RSP=%d REG_RBP=%d REG_RDI=%d REG_RSI=%d\n",
         offsetof(ucontext_t, uc_mcontext), REG_RIP, REG_RSP, REG_RBP, REG_RDI, REG_RSI);
#else
  printf("platform=unsupported-for-x86-acceptance\n");
#endif
  return 0;
}
EOF
  cc "${ref_c}" -o "${ref_bin}"
  "${ref_bin}"
  rm -f "${ref_c}" "${ref_bin}"

  echo "== build compiler =="
  (cd .. && go build -o bin/elisacore ./compiler/src)

  echo "== escape-hatch ratchet =="
  tools/unsafe_ratchet.sh

  echo "== guest exec native tests =="
  ELISACORE_TEST_CACHE=0 ../bin/elisacore test emulator-guest-exec-runtime-tests --project .

  echo "== guest exec x64 tests =="
  ELISACORE_TEST_CACHE=0 ../bin/elisacore test emulator-guest-exec-runtime-tests-x64 --project .

  echo "== CUSA07399 x64 exec =="
  python3 emulator_cusa07399_x64_exec.py

  echo "== parity require first boundary =="
  ELISACORE_TEST_CACHE=0 ./emulator-cpp-parity --require-first-boundary --require-x64-real-exec

  echo "GUEST_EXEC_X86_ACCEPTANCE status=ok finished=$(date -u '+%Y-%m-%dT%H:%M:%SZ')"
} 2>&1 | tee "${OUT}"
