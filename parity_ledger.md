# Parity Ledger

This ledger is the source of truth for **active port targets** and their
parity state against upstream C++.

Ledger file:

- `parity_ledger.json`

Guard checker:

- `python3 parity_ledger_check.py --summary`
- `python3 parity_abi_check.py --summary`

## Status Values

- `Implemented`: behavior is ported to Elisa.
- `Stub-Parity`: upstream C++ is currently stub/no-op; Elisa intentionally matches it.
- `External-C-ABI`: boundary is allowed native code (external library or OS/runtime API) behind C ABI.
- `Missing`: not yet at parity.

## Guard Rules

`parity_ledger_check.py` fails when:

- any required symbol for an active target is not classified,
- an entry has an invalid status,
- an `Implemented` entry has no parity test tag,
- an `External-C-ABI` entry is not classified as `external-library` or `os-runtime-api`,
- an `External-C-ABI` entry is missing the owning shim.
  - exception: `temporary-project-bridge-approved` is allowed only for explicitly documented transitional bridges.

This enforces:

- no unclassified symbols for active targets,
- no project-owned logic hidden behind approved external C ABI categories,
- explicit test coverage tags for implemented behavior.

`parity_abi_check.py` additionally enforces:

- explicit `@c_abi(c)` on active-target `External-C-ABI` externs,
- fixed-layout markers for active-target POD-like structs crossing those boundaries.
