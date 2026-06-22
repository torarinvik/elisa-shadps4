# E2 fit finding - ByteReader protocol contracts

## Decision: declined

The requested `ByteReader` contract does not currently earn its place in this tree/compiler.

- Existing file-format reads are already operationally bounded by `ff_can_read`; underflow returns `0`
  or clamps the cursor instead of trapping.
- Elisa protocols in this checkout are marker/type-family concepts. A probe accepted protocol
  declarations, but a constrained generic `R: ProbeReader` could not call a required operation via
  either `r.remaining()` or `remaining(r)`, so a behavioral `ByteReader` protocol would not supply
  real generic dispatch or proof obligations.
- The installed `elisac` does support `requires`/`ensure`, and `old(...)` parses, but `changes` is
  not a recognized contract keyword. The requested `changes self.cursor` violation test therefore
  cannot be expressed against this compiler.
- Without protocol method contracts and `changes`, the proposed generic `read_header[R]` would be
  decorative: it would not prove the second/third read preconditions through protocol ensure
  injection, would not elide a runtime `ff_can_read`, and would not bound a mutation surface beyond
  what the current reader already does operationally.

No protocol/law/refinement was added.

## Verification probes

Run in project root with `ELISACORE_TEST_CACHE=0 elisac -emit semantic _e2_probe.elisa`:

- Protocol + method dispatch probe rejected generic calls:
  - `r.remaining()` failed with `field access requires struct type, got R`.
  - `remaining(r)` failed because the resolved function expected `ProbeImpl&`, not `R&`.
- `changes self.cursor` probe failed with `undefined identifier "changes"`.
- `ensure self.cursor == old(self.cursor) + 1` parsed, but the proof failed on unsigned overflow,
  confirming `old(...)` exists while `changes` does not.

---

## UPDATE (2026-06-22): decline RESOLVED — E2 now lands

The root blocker ("a constrained generic `R: ProbeReader` could not call a required
operation via `r.remaining()`") was misattributed to "protocols are marker-only". The
real cause, isolated on retry: generic protocol-method dispatch worked through a
by-VALUE receiver (`r: R`) but NOT through a REFERENCE receiver (`r: R&` / `mutable R&`),
failing with `field access requires struct type, got R`. Byte readers fundamentally need
a mutating `mutable R&`, so by-value was no workaround.

Fixed in the compiler (`rewriteBoundTypeParamMethodCall` now unwraps the receiver
reference before the bound-type-param match). With that, the `ByteReader` protocol was
implemented and lands cleanly:
- `core/file_format/byte_reader.elisa`, `elisa_tests/byte_reader_protocol_tests.elisa`
- 5/5 (4 @test + 1 @property proving FileFormatReader and SliceReader agree).

The separate `changes` claim in the original decline was also stale — `changes` in the
indented protocol-method block parses (A2 covered it).

See `verification_findings/PENDING_TARGETS/E2-byte-reader-protocol.md`.
