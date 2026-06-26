# 100 Tiny Gains — Emulator Cleanup Checklist

## Progress
- ✅ **Category A (#1–53) DONE** — `f942ce8`: 3989/3998 numeric-suffix warnings
  removed across 47 files in one verified sweep (9 load-bearing suffixes
  preserved: u64 literals that overflow `i64`, and `id[...]`-cast feeders).
  All 47 targets verified clean vs baseline.
- ⏭️ **Category B (#54–61) SKIPPED (audited, intentional)** — all 8 are the
  same correct pattern: build a NUL-terminated buffer in a persistent arena,
  then `return out[0].cast[static u8&]` (a C-string contract). Already inside
  `trusted Unsafe.*` with vouching comments. The linter flags the index-rooted
  cast for audit; it has been audited. No tiny fix exists — clearing it needs
  either a return-type change (ripples to all callers) or a centralizing helper
  that only relocates the warning (8→1) while touching working FFI. Not worth
  the risk under a "tiny safe gains" mandate. Left as-is.
- ⬜ Category C (#62–71) — struct padding
- ⬜ Category D (#72–100) — effect-grant `can` blocks


Momentum-first list of the **easiest** things to fix. No new features — only
lint cleanups, safety smells, and layout nits surfaced by the compiler.

**Surface (from `elisacore -emit semantic main.elisa`):**
- 3998 `numeric literal suffix discouraged` warnings across 47 files
- ~600 `requires can[...] — no explicit local effect grant` warnings across 17 files
- 8 `casting an interior pointer ... erases its length` (OOB cstring) smells
- 10 `avoidable padding` struct-layout warnings

> There is **no** real TODO/FIXME backlog — the grep hits were all false
> positives (`scePlayGoGetToDoList`, the IME `placeholder` field, C++ symbol
> names containing "Placeholder"/"Document").

**Workflow per item:** fix → `elisacore test <target> --project .` (or
`-emit semantic`) → confirm **no new errors vs baseline** (some "tiny" fixes
have non-local effects — see cautions). Commit in small batches.

---

## A. Numeric-suffix cleanup — SAFEST, do these first (#1–53)

Mechanical: drop the discouraged literal suffix where context infers the type,
or use `.cast[T]`. ⚠️ Removing a suffix can change the inferred type — always
recompile the file's target and confirm zero new errors.

Smallest files first (quick wins / momentum):

1. `core/libraries/video_recording/video_recording_nids.elisa` (1)
2. `core/libraries/kernel/coredump/coredump_nids.elisa` (2)
3. `core/libraries/voice/voice.elisa` (3)
4. `core/libraries/kernel/threads/threads_sema_spec.elisa` (3)
5. `core/libraries/ulobjmgr/ulobjmgr_nids.elisa` (4)
6. `core/libraries/system/userservice.elisa` (5)
7. `core/libraries/np/np_manager.elisa` (6)
8. `core/address_space.elisa` (7)
9. `core/libraries/hmd/hmd_setup_dialog_nids.elisa` (7)
10. `core/libraries/kernel/time.elisa` (7)
11. `core/libraries/sysmodule/sysmodule.elisa` (7)
12. `fixtures/boot/tiny_eboot_elf.elisa` (7)
13. `core/libraries/playgo/playgo_hle_module.elisa` (8)
14. `core/libraries/generated_hle_nids.elisa` (10)
15. `core/libraries/kernel/equeue.elisa` (12)
16. `core/libraries/libc_internal/libc_internal_memory.elisa` (12)
17. `core/libraries/mouse/mouse_nids.elisa` (14)
18. `core/libraries/hmd/hmd_distortion_nids.elisa` (16)
19. `core/libraries/kernel/kernel.elisa` (19)
20. `core/libraries/kernel/threads/threads.elisa` (22)
21. `core/libraries/kernel/threads/threads_event_exception_clean.elisa` (23)
22. `core/libraries/ime/ime_nids.elisa` (23)
23. `core/libraries/share_play/share_play_nids.elisa` (25)
24. `core/libraries/kernel/memory.elisa` (26)
25. `core/libraries/videoout/videoout_nids.elisa` (27)
26. `core/libraries/kernel/process.elisa` (31)
27. `core/libraries/hmd/hmd_reprojection_nids.elisa` (35)
28. `core/hle_libs.elisa` (36)
29. `input/controller.elisa` (41)
30. `core/libraries/remote_play/remote_play_nids.elisa` (42)
31. `core/libraries/audio/audioin_nids.elisa` (42)
32. `core/tls.elisa` (47)
33. `core/libraries/kernel/threads/kt_runtime.elisa` (48)
34. `core/libraries/game_live_streaming/gamelivestreaming_nids.elisa` (51)
35. `core/memory.elisa` (61)
36. `core/module.elisa` (70)
37. `core/libraries/audio/audioout_nids.elisa` (71)
38. `core/libraries/pad/pad.elisa` (78)
39. `core/libraries/kernel/threads/threads_sync.elisa` (91)
40. `core/libraries/save_data/savedata_nids.elisa` (95)
41. `core/libraries/hmd/hmd_nids.elisa` (137)

Big files — split by line range (do per batch, recompile between):

42. `core/loader/elf.elisa` lines 15–427 (~87)
43. `core/loader/elf.elisa` lines 427–764 (~87)
44. `core/linker.elisa` lines 33–1094 (~115)
45. `core/linker.elisa` lines 1094–2522 (~116)
46. `core/libraries/gnmdriver/gnmdriver.elisa` lines 56–1138 (~165)
47. `core/libraries/gnmdriver/gnmdriver.elisa` lines 1138–1617 (~164)
48. `core/libraries/kernel/kernel_nids.elisa` lines 17–627 (~306)
49. `core/libraries/kernel/kernel_nids.elisa` lines 627–1239 (~306)
50. `core/guest_exec.elisa` lines 73–1554 (~338)
51. `core/guest_exec.elisa` lines 1554–6214 (~339)
52. `core/libraries/network/network_nids.elisa` lines 17–717 (~351)
53. `core/libraries/network/network_nids.elisa` lines 717–1421 (~352)

## B. OOB cstring-cast smells — small, isolated, MEDIUM (#54–61)

Each casts a sized `darray` interior pointer to an unbounded `static u8&`,
erasing length (can read past the buffer). Replace with a length-bounded view
(`sview`) or copy bounded bytes. Verify behavior unchanged.

54. `core/guest_exec.elisa:81`
55. `core/module.elisa:818`
56. `core/module.elisa:845`
57. `core/module.elisa:935`
58. `core/linker.elisa:765`
59. `core/linker.elisa:782`
60. `emulator.elisa:122`
61. `emulator.elisa:137`

## C. Struct padding reorders — MEDIUM, ⚠️ check layout-sensitivity (#62–71)

Reorder fields to cut padding. ⚠️ Some structs are guest-facing / memory-mapped
/ serialized — reordering changes layout and can break ABI. Confirm the struct
is host-only before reordering; otherwise skip.

62. `launch_cli.elisa:13` — `CliState` (→96 B)
63. `core/user_manager.elisa:13` — `User` (→64 B)
64. `core/module.elisa:79` — `ImportSymbolRecord` (→64 B)
65. `core/module.elisa:124` — `Module` (→616 B) ⚠️ likely layout-sensitive
66. `core/linker.elisa:148` — `SymbolRecord` (→56 B)
67. `core/linker.elisa:196` — `Linker` (→288 B) ⚠️ likely layout-sensitive
68. `core/libraries/ajm/ajm_batch.elisa:36` — `AjmParsedJobSummary` (→136 B)
69. `input/controller.elisa:64` — `InputState` (→192 B)
70. `input/controller.elisa:77` — `InputGameController` (→13112 B)
71. `emulator.elisa:22` — `Emulator` (→168 B)

## D. Effect-grant `can`-blocks — MEDIUM, ⚠️ propagates to callers (#72–100)

Add the missing `can X:` grant (or `trusted`) where the warning points. ⚠️ A
`can X:` block adds the effect to the **function signature** and propagates to
callers — this has broken builds before. Do per-file with the verify-loop and
watch for new caller errors. Split the big files into batches.

72. `core/libraries/kernel/threads/threads_sync.elisa` — batch 1 (149 total)
73. `core/libraries/kernel/threads/threads_sync.elisa` — batch 2
74. `core/libraries/kernel/threads/threads_sync.elisa` — batch 3
75. `core/libraries/kernel/threads/threads_sync.elisa` — batch 4
76. `core/libraries/kernel/threads/kt_runtime.elisa` — batch 1 (138 total)
77. `core/libraries/kernel/threads/kt_runtime.elisa` — batch 2
78. `core/libraries/kernel/threads/kt_runtime.elisa` — batch 3
79. `core/libraries/kernel/threads/kt_runtime.elisa` — batch 4
80. `core/guest_exec.elisa` — batch 1 (88 total)
81. `core/guest_exec.elisa` — batch 2
82. `core/guest_exec.elisa` — batch 3
83. `core/linker.elisa` — batch 1 (85 total)
84. `core/linker.elisa` — batch 2
85. `core/linker.elisa` — batch 3
86. `core/module.elisa` — batch 1 (38 total)
87. `core/module.elisa` — batch 2
88. `core/libraries/kernel/threads/threads_sema_spec.elisa` (23)
89. `core/libraries/kernel/threads/threads_event_exception_clean.elisa` (12)
90. `core/libraries/kernel/threads/threads.elisa` (11)
91. `core/libraries/kernel/memory.elisa` (11)
92. `core/memory.elisa` (9)
93. `core/libraries/kernel/process.elisa` (9)
94. `core/libraries/libc_internal/libc_internal_memory.elisa` (6)
95. `emulator.elisa` (4)
96. `core/libraries/kernel/kernel.elisa` (4)
97. `launch_pipeline.elisa` (2)
98. `core/tls.elisa` (2)
99. `main.elisa` (1)
100. Final sweep: rebuild `main.elisa`, confirm the effect-grant warning count is 0.
