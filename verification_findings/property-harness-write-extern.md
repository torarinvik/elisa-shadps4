# @property harness vs custom-runtime `write` binding

Finding: the property-test harness emits compiler-internal externs `__prop_snprintf`/`__prop_write`
(link_name "snprintf"/"write") to print failing inputs. In a project whose runtime rebinds `write`
with a different nominal signature (shadPS4: `runtime_write(FileDescriptor, void&, usize) can[Console]`
vs the harness's `__prop_write(int, void&, usize)`), the "native symbol splitting" check rejected the
generated test runner — so `@property` could not run in this (or any custom-runtime) project.

## RESOLVED (compiler fix, 2026-06-22)
The splitting diagnostic now skips when either declaration is a harness-internal `__prop_*` extern
(ABI-correct by construction; nominal-only mismatch). `@property` now runs in custom-runtime projects.
Verified: elisa_tests/core_file_format_psf_fuzz_properties.elisa (`@property psf_parse_never_traps`)
runs 1/1. The non-internal splitting error is unchanged (extern ABI tests still green).
