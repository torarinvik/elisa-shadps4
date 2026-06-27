#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

ELISAC_BIN="${ELISAC:-}"

if [[ -z "$ELISAC_BIN" ]]; then
  if [[ -n "${ELISA_CORE:-}" ]]; then
    ELISAC_BIN="go run ./src"
    COMPILER_DIR="$ELISA_CORE/compiler"
  elif [[ -d "$ROOT/../Elisa-core/compiler" ]]; then
    ELISAC_BIN="go run ./src"
    COMPILER_DIR="$ROOT/../Elisa-core/compiler"
  elif [[ -d "$ROOT/../../Go projects/Elisa-core/compiler" ]]; then
    ELISAC_BIN="go run ./src"
    COMPILER_DIR="$ROOT/../../Go projects/Elisa-core/compiler"
  else
    echo "Set ELISAC to an elisac binary or ELISA_CORE to an Elisa-core checkout." >&2
    exit 2
  fi
fi

run_elisac() {
  if [[ -n "${COMPILER_DIR:-}" ]]; then
    (cd "$COMPILER_DIR" && $ELISAC_BIN "$@")
  else
    $ELISAC_BIN "$@"
  fi
}

TMP_DIR="$(mktemp -d "${TMPDIR:-/tmp}/refined-register-lowering.XXXXXX")"
trap 'rm -rf "$TMP_DIR"' EXIT

CORE_LOWERED="$TMP_DIR/shader_recompiler_ir_core_pure_tests.lowered.elisa"
IR_EMITTER_LOWERED="$TMP_DIR/ir_emitter.lowered.elisa"

run_elisac -emit lowered -o "$CORE_LOWERED" "$ROOT/elisa_tests/shader_recompiler_ir_core_pure_tests.elisa"
run_elisac -emit lowered -o "$IR_EMITTER_LOWERED" "$ROOT/shader_recompiler/ir/ir_emitter.elisa"

python3 tools/check_refined_register_lowering.py "$CORE_LOWERED"
python3 elisa_tests/audit_lowered_register_bounds.py \
  --core-lowered "$CORE_LOWERED" \
  --ir-emitter-lowered "$IR_EMITTER_LOWERED" \
  --write-report verification_findings/lowered-register-bounds-audit.md
