# file_sys C++ vs Elisa Parity Matrix

Status values:
- `Exact`: behavior intended to match current C++ behavior.
- `Intentional Divergence`: Elisa behavior differs by design and is documented.
- `Deferred`: known gap not yet completed.

## Mount + path resolution
- Longest-prefix mount selection: `Exact`
- Read-only propagation on resolved mount: `Exact`
- Overlay path typing (base/mod/patch selection): `Exact`
- Path normalization + trailing slash policy: `Exact`
- Case-insensitive host fallback: `Exact`
- Host sidecar filtering (`.DS_Store`): `Exact`

## Directory + getdents
- Dirent packing/alignment/null-termination: `Exact`
- Partial-buffer continuation semantics: `Exact`
- Stable rebuild and emit ordering: `Exact`
- `NormalDirectory` / `PfsDirectory` parity over shared packer: `Exact`

## Handle table + file handle
- Descriptor creation/reuse/invalid guards: `Exact`
- `lseek` whence and invalid input handling: `Exact`
- open/read/write/pread/pwrite/truncate/flush/unlink surface: `Exact` (behavioral parity target)

## Devices
- `NopDevice` contracts (`ioctl`, read/write family, seek/stat/fsync/truncate/getdents): `Exact`
- `RandomDevice` (`/dev/random` style) stat/read behavior: `Exact`
- `RngDevice` ioctl contract + random fill behavior: `Exact`
- `SRandomDevice` / `URandomDevice` return-code matrix: `Exact`
- Console/DeciTty6 stub-return behavior: `Exact` (stub parity)
- Logger buffering/flush semantics: `Exact`

## Deferred / outstanding
- None currently identified in `core/file_sys` parity suites and emit gates.

## Validation gates
- `go run ./src ../elisa-shad-ps4-from-scratch/elisa_tests/core_file_sys_pure_tests.elisa`
- `go run ./src ../elisa-shad-ps4-from-scratch/elisa_tests/core_file_sys_devices_pure_tests.elisa`
- `go run ./src -emit llvm ../elisa-shad-ps4-from-scratch/core_port.elisa`
