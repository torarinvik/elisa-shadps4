# Differential boot oracle (verification #5)

Elisa's in-language verification (contracts, refinement types, `@property`) cannot reach the
native/FFI/guest-exec layer — `extern` bodies, raw guest memory, the x86-64 dispatcher. That
boundary is pinned instead by **differential equivalence against a proven reference**: run the
SAME PS4 game through shadPS4 and the from-scratch Elisa emulator, normalize each boot trace to
a common event schema (entry offset, module-start fallback sequence), and report the FIRST
divergence — which is, by construction, where the Elisa emulator first behaves differently.

This is the reusable form of the comparison that re-diagnosed "CRASH #2" (entry value + segment
load proven correct against shadPS4 → the bug is post-load guest memory).

    python3 difftest/boot_difftest.py \
        --shadps4 ../shadPS4/build-elisa-x86_64/shadps4 \
        --game ../shadPS4/Games/CUSA07399/eboot.bin \
        --elisa-runner <kept x64 test runner>

Build the Elisa runner with:
    ELISA_KEEP_TEST_BINARY=1 (cd ../compiler && go run ./src test emulator-cusa07399-x64-exec \
        --project ../elisa-shad-ps4-from-scratch -target-triple x86_64-apple-darwin)

In-language differential testing (two Elisa implementations) needs no external tool — it is a
plain `@property`:

    @property
    def fast_matches_reference(x: i32) -> bool:
        return fast_impl(x) == reference_impl(x)
