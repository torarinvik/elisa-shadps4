# PENDING project.json target stanza — E2 ByteReader protocol (retry, now landed)

The earlier E2 declined the `ByteReader` protocol because generic protocol-method
dispatch through a *reference* receiver (`r: mutable R&`) did not resolve — only
by-value `r: R` worked. That was a real compiler gap (not a "protocols are marker-only"
limitation), now fixed in the compiler:

  fix(semantic): generic protocol-method dispatch through a reference receiver
  (rewriteBoundTypeParamMethodCall now unwraps the receiver reference before the
   bound-type-param match).

With that fix the protocol earns its place: one set of reader-agnostic combinators
serves two reader representations (`FileFormatReader`, `SliceReader`) through a
mutating `mutable R&` receiver.

## Files added (source-only, additive — nothing existing includes these)
- `core/file_format/byte_reader.elisa` — `protocol ByteReader` (`br_remaining`,
  `br_next_u8`), impls for `FileFormatReader` and a zero-alloc `SliceReader`, and
  the generic `br_read_u16_le/be`, `br_read_u32_le/be` combinators.
- `elisa_tests/byte_reader_protocol_tests.elisa` — 4 `@test` + 1 `@property`.

## Verified standalone
`ELISACORE_TEST_CACHE=0 elisac -emit test elisa_tests/byte_reader_protocol_tests.elisa`
=> `passed=5 skipped=0 failed=0` (incl. `__property_fuzz_byte_reader_impls_agree_u32_le`,
which proves the two impls are observationally identical over arbitrary 4-byte words).

## Proposed test-target stanza (project.json `targets`, when wired)
```json
"byte-reader-protocol-tests": {
  "entry": "elisa_tests/byte_reader_protocol_tests.elisa",
  "emit": "test",
  "foreign": []
}
```

## Notes
- Past-EOF reads stay total for both impls (operational bound, mirroring `ff_read_u8`);
  `byte_reader_past_eof_is_total` covers this.
- No `requires`/`changes` contract was forced onto the protocol methods: the genuine
  win here is the by-ref generic abstraction over 2 representations, not a proof
  obligation. Adding a `requires`/`ensure` would have been decorative (fit test).
