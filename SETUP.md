# Setup — Elisa compiler runtime dependency

This repo's `.elisa` sources `include` the Elisa-core **compiler runtime**
(`compiler/runtime/elisacore_std/…`) via relative paths that assume `compiler/` is
reachable as `<this repo's parent>/compiler/`. The repo was originally authored *inside*
the Elisa-core checkout; it now lives under `Elisa Projects/`, so that reference must be
re-pointed at the sibling Elisa-core checkout.

The 123 include sites span many depths (`../compiler/…`, `../../compiler/…`,
`../../../../compiler/…`) and include generated `*.lowered.elisa` files, so rather than
rewrite them all, the runtime is resolved through a single symlink in the parent folder:

```
Elisa Projects/compiler  ->  ../Go projects/Elisa-core/compiler
```

## Recreate the symlink (idempotent)

From the `Elisa Projects/` folder (the parent of this repo):

```sh
ln -sfn "../Go projects/Elisa-core/compiler" compiler
```

Adjust the target if your Elisa-core checkout lives elsewhere. After this, the sources
semantic-check from this location, e.g.:

```sh
cd "<Elisa-core>/compiler"
go run ./src -strict -emit semantic "<this repo>/core_libraries_audio3d_integration.elisa"
```

(The Wolf3D Elisa port uses direct relative includes instead, since it has only a handful
of include sites; this repo's scale made the single symlink the lower-risk choice.)

## Refined register lowered regression

The repeatable lowered-output guard for refined SGPR/VGPR register bounds is:

```sh
tools/run_refined_register_lowering_regression.sh
```

Set `ELISAC=/path/to/elisac` to use a prebuilt compiler, or set `ELISA_CORE=/path/to/Elisa-core`
to run it from source. The script refreshes the focused lowered outputs, runs
`tools/check_refined_register_lowering.py`, and regenerates
`verification_findings/lowered-register-bounds-audit.md`.
