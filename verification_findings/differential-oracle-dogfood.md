# Dogfood: @differential oracle harness on address-space predicates

The new first-class `@differential` harness (thin layer over `@property`: deterministic
xorshift64 fuzz inputs + greedy shrinker + counterexample report) was dogfooded against the
emulator's pure address-space predicates in `core/address_space.elisa`.

## What landed
`elisa_tests/differential_address_space_tests.elisa` — two differentials, each checking an
emulator predicate against an INDEPENDENTLY-computed reference spec:
- `diff_page_aligned`: `AddressSpace_PageAligned` (bitmask `& 0xFFF`) vs `value % 4096 == 0`.
- `diff_range_no_overflow`: `AddressSpace_RangeNoOverflow` (wraparound `base+size >= base`) vs
  the no-wrap test `size <= U64_MAX - base`.

Both report `[ OK ]` (`-emit test`, 256 fuzzed cases each).

## Mechanism is sound (dense-divergence catch)
A deliberately negated reference over `AddressSpace_RangesOverlap` (disagrees on the vast
majority of inputs) was caught at **case 0** and shrunk to the minimal counterexample
`(ab=0, asz=0, bb=0, bsz=0)`. The harness, shrinker, and counterexample reporting all work
end-to-end on real emulator code.

## FINDING: uniform scalar fuzzing misses sparse domains (boundary-biased generation needed)
Injecting a real page-alignment bug into the reference (`% 4096` -> `% 2048`) did **NOT** fail
the differential. Reason: uniform-random u64 is essentially never page-aligned, so both the
impl and the (buggy) reference return `false` on virtually every drawn input and agree
**vacuously**. The divergence lives only on the *aligned* inputs, a ~1/2048-density subset the
generator almost never produces. The same vacuity affects overflow-boundary predicates (random
`base,size` essentially never wrap).

This is the central limitation for emulator dogfooding specifically: the emulator is saturated
with alignment / boundary / mask predicates whose *interesting* domain is sparse, and uniform
fuzzing is blind to exactly those.

### Recommended follow-up for the harness (E)
Add **boundary-biased input generation**: with some probability draw each scalar from a
structured edge-case set instead of uniformly —
- 0, 1, 2; type-MAX, MAX-1, MAX-k
- powers of two and (2^k - 1) masks
- page-aligned values (`n << 12`) and page-aligned ± 1
- for paired range params, values near each other / overlapping / exactly adjacent

A mixed distribution (≈50% biased, 50% uniform) would have caught the `% 2048` bug immediately
while preserving broad coverage. This mirrors how mature property-based fuzzers (QuickCheck
`frequency`, proptest's int strategies) seed boundary values. Until then, author differentials
so the interesting domain is reachable (e.g. generate an aligned value as `(raw << 12)` inside
the body rather than testing `raw` directly).

## UPDATE — boundary-biased generation LANDED (compiler)
The `@property`/`@differential` generator now draws ~50% from a structured edge-case set per
integer type (0/1/2, MAX/MAX-1/MAX-k, `1<<k`, `(1<<k)-1`, page `n<<12`, page±1), deterministic
and shrink-compatible. Dogfooded back on the address-space differentials:
- The CORRECT `diff_page_aligned` (`AddressSpace_PageAligned` vs `% 4096`) now passes
  NON-VACUOUSLY — page-aligned inputs are actually exercised (previously never were).
- A realistic page-domain bug — impl masking `& 0x1FFF` (8192) instead of `& 0xFFF` (4096),
  i.e. diverging on odd multiples of 4096 — is now CAUGHT at case 18 (shrunk to an odd 4096
  multiple). The whole class of "wrong alignment mask / wrong page size" bugs is now reachable.

RESIDUAL (characterized): a divergence localized to ONE specific power of two (e.g. exactly
`2048 == 1<<11`, or odd-multiples-of-2048 from a `%4096→%2048` bug) is still diluted — the
`1<<k` category spreads probability across all 64 widths, so any single power appears <1× per
256 cases. The PAGE category densely covers all 4096-multiples (the emulator-relevant domain),
so page-size/alignment-mask bugs are well covered; sub-page single-power divergences are not.
Reinforces the strategic split: refinement TYPES (`Aligned[N]`) PROVE alignment for all inputs
(no coverage gap); biased fuzzing is the best-effort falsification complement. A possible
follow-up is to bias `k` toward common shifts (12/11/6/3) so specific small powers recur.
