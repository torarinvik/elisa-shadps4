# Guest Exec Rosetta Verification Finding

2026-06-22 A1: `/usr/bin/arch -x86_64 /usr/bin/true` returns 0 and `/usr/bin/arch -x86_64 /usr/bin/uname -m` prints `x86_64`, so Rosetta subprocess launch is available on this arm64 macOS host.
Repro: `ELISACORE_TEST_CACHE=0 elisac test emulator-guest-exec-runtime-tests-x64 --project .`, `elisac -O0 -emit test -target-triple x86_64-apple-darwin -filter x64_subprocess elisa_tests/emulator_guest_exec_runtime_tests.elisa`, and the same `-O3` command all return rc=1 before runtime.
Current blocker is non-owned strict proof debt only: `core/memory.elisa:321` (`MemoryManager_SetupMemoryRegions`) and `core/module.elisa:712` (`Module_AlignUp`); no `core/guest_exec.elisa` ensure/undefined diagnostics remain after the literal-return rewrite.
