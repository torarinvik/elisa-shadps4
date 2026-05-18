# Extern Ledger

This ledger classifies every remaining Elisa `extern` in `common/`.
Project-owned C++ logic must move into Elisa. Native code is allowed only for
OS/runtime APIs, compiler intrinsics, existing C APIs, or C wrappers around
external C++ libraries.

Run:

```sh
python3 extern_ledger.py --summary
```

Categories enforced by `extern_ledger.py`:

- `project-owned-to-port`: temporary externs for project-owned code that must be ported to Elisa.
- `external-c-api`: linked third-party APIs with a C-compatible surface.
- `external-cpp-shim`: linked C++ libraries or runtime facilities that need repo-owned `extern "C"` shims.
- `os-c-api`: OS, libc, clock, thread, atomics, or debug runtime boundary.
- `compiler-intrinsic`: LLVM or target CPU intrinsic.
- `build-metadata`: build-system injected version metadata.
- `windows-wide-string-shim`: Windows UTF conversion boundary.

The completion bar is: no unclassified externs, no C++ ABI exposed to Elisa,
and no `project-owned-to-port` symbols left unless they have been split down
to true OS/library boundaries.
