# Port provenance: keeping the Elisa port verifiably in sync with shadPS4

The shadPS4 repo (`../shadPS4`) is the single C++ source of truth. To make "this
Elisa code is a faithful port of that C++" **checkable** (not just asserted), each
ported function is anchored to its C++ origin and a tool verifies the anchor stays
valid as shadPS4 evolves.

We deliberately do **not** copy the C++ body into the Elisa source (that re-creates
a second source of truth that silently drifts). Instead we record a *content hash*
of the oracle symbol and re-check it.

## The `@port` anchor

Put one line directly above a ported function:

```
# @port core/libraries/kernel/process.cpp::sceKernelGetCompiledSdkVersion sha256=92b20e66de8390d2 oracle@a3773841
def sceKernelGetCompiledSdkVersion(ver: void&?) -> int:
    ...
```

- `core/libraries/kernel/process.cpp` — path of the C++ definition, relative to
  `shadPS4/src`.
- `sceKernelGetCompiledSdkVersion` — the C++ symbol (function) ported here.
- `sha256=...` — hash of that C++ symbol's source when the port was last verified.
- `oracle@...` — the shadPS4 commit at that time (informational).

You write only the first part; the tool fills in `sha256=`/`oracle@`.

## Workflow

Add an anchor, then stamp it from the current oracle:

```bash
# 1. add `# @port <relpath>::<symbol>` above the ported def
# 2. stamp the hash + oracle commit:
python3 tools/port_provenance.py --update
# 3. commit the .elisa change
```

Check (CI gate; exits nonzero on any problem):

```bash
python3 tools/port_provenance.py            # report problems only
python3 tools/port_provenance.py --summary  # also print OK/DRIFTED/BROKEN counts
```

## Bulk anchoring (retroactive)

To anchor the existing port in one pass:

```bash
python3 tools/port_provenance.py --autoanchor   # add anchors (dry-run with --dry-run)
python3 tools/port_provenance.py --update        # stamp them
python3 tools/port_provenance.py --summary       # verify all OK
```

`--autoanchor` adds an anchor above every Elisa `def` whose name is PS4-HLE-style
(`sce*`, `_sce*`, `posix_*`) and that has exactly one matching definition in the
oracle (found via a fast `rg` pass). It inserts above any decorators (never between
a decorator and its def) and skips overloaded/ambiguous symbols. Because PS4 HLE
names are globally unique (NID-mapped), a name match is an accurate *provenance +
drift-tracking* link — it does not by itself certify the port is bug-free, only
"this ports that; alert me if that C++ changes." Hand-verified review of a function
is recorded the same way: re-read the C++, fix the port if needed, `--update`.

## What the checker reports

For each anchor it re-extracts the named C++ symbol from the current oracle, hashes
it, and compares to the recorded hash:

- **OK** — hash matches: the port is still in sync with the C++ it was verified against.
- **DRIFTED** — the C++ changed since the port was verified. Re-read the new C++,
  update the Elisa port if needed, then `--update` to re-stamp (the re-stamp is your
  signed statement "I re-verified the port against this new C++").
- **BROKEN** — the oracle file/symbol is missing, ambiguous (overloaded — name a more
  specific target), or the anchor is unstamped (`--update`). Fix the anchor.

Only EOL / trailing-whitespace differences are tolerated; any real C++ change flips
the hash, so drift is never missed and is never a false alarm from reformatting.

## When to anchor, and what else to add

- **Anchor every ported function** with `@port`. That is the verifiable, low-drift
  backbone — it scales to thousands of functions because the *tool* re-verifies, and
  it never goes stale silently.
- **Inline a C++ snippet only where the C++ is the spec** — reverse-engineered bit
  math, struct layouts, magic constants, subtle control flow. Use a `""" ... """`
  block so the exact source sits next to the port where it genuinely prevents bugs.
  Do **not** mirror boilerplate (a plain `vector` → `darray` declaration gains nothing
  from a duplicated comment and just adds drift surface).

This keeps shadPS4 as the one source of truth, makes faithfulness continuously
checkable, and avoids drowning every file in stale duplicate C++.
