# trusted audit: scatter clusters → missing safe primitives

The `can`-over-`trusted` migration converted ~150 `trusted` blocks to `can` (propagate the
unsafe effect upward) and kept ~58 at genuine leaf/boundary sites. The audit surfaced these
SCATTER CLUSTERS — the same effect firewalled in many near-identical sites, which is the signal
of a MISSING shared safe primitive (N firewalls that should be ONE audited helper):

1. **`Unsafe.GuestHostPointerCast` — ~22 sites** (threads_sync `*_hle` boundary wrappers).
   Each does `trusted ...: return inner(addr.cast[...])` at the guest↔host address crossing.
   → one `boundary_cast[T](GuestVAddr) -> T&?` helper (or codegen from `@boundary_pointer_args`).

2. **`Unsafe.PointerCast` `*_from_handle` converters — ~8 sites** (kt_runtime KtMutex/Cond/
   Rwlock/Sem/Event/Thread + `mutex_as_handle`). Opaque-`void&?`→typed-ref leaves.
   → one generic `handle_as[T]` / `as_handle` primitive.

3. **`Unsafe.StaleRef` "store an externally-owned handle into a long-lived slot" — ~18+ sites**
   (avplayer, videodec, videoout, font, kernel, np, threads). Same invariant everywhere.
   → a documented `store_runtime_owned` / `store_borrowed_handle` primitive.

4. **arena-cstr builder — 5+ sites** (module `Module_CopySlice`/`CopyDynDataSlice`/`EncodeId`,
   linker `Linker_Substr`/`Concat2`, verbatim_port). `build darray in arena; return &out[0]`.
   → one `arena_cstr_from_view` primitive (which would then be the single KEEP firewall).

5. **cxa-guard byte-poke — 3 sites** (hle_libs CxaGuardAcquire/Release/Abort), same
   `guard: void&? → mutable u8&` cast. → one `cxa_guard_byte(guard)` accessor.

6. **`Unsafe.UncheckedIndex` cstr indexing — ~8 sites** (emulator path/string helpers, bounded
   by `static_strlen`). → a length-carrying bounded-cstr-slice view.

7. **`Unsafe.AssumeProgress` zlib queue/ring loops — 7 sites** (zlib_async_bridge).
   → a shared bounded-ring iteration primitive.

Each consolidation collapses many `trusted`/`can` sites into one audited helper — the durable
win an automated `trusted`-cluster lint (docs/102 §"trusted audit") would flag continuously.
