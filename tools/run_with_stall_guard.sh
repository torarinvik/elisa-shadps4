#!/usr/bin/env bash
# Stall guard for the from-scratch emulator boot.
#
# Why this exists: a guest busy-spin (a tight native loop polling memory, making no HLE calls
# and never faulting) cannot be caught by our in-process instrumentation — the HLE call ring
# records no new crossings, and the SIGALRM emergency watchdog does not fire into Rosetta-
# translated guest code (see verification_findings/gap3-reframe-guest-spin.md). So a hang would
# silently burn the full outer timeout (~6 min) with no diagnosis.
#
# This wraps `elisac test <target>` from OUTSIDE the process, where neither Rosetta nor signal
# delivery matters: it watches boot progress (log growth), and on a stall it classifies the
# hang (busy-spin vs blocked-wait via CPU%), captures the hot stack with `sample`, prints the
# last runtime lines, then kills cleanly and exits non-zero — turning a 6-min silent hang into
# a ~20s fail-fast with a diagnosis.
#
# Usage:   tools/run_with_stall_guard.sh [target]
#   env:   STALL_SECS (default 20)   any ELISA_* vars are passed through to the boot
# Exit:    rc of the target if it completes; 2 on stall; 3 on setup error.
set -u

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TARGET="${1:-emulator-cusa07399-x64-exec}"
STALL_SECS="${STALL_SECS:-20}"
ELISAC="${ELISAC:-$HOME/.local/bin/elisac}"
LOG="/tmp/stallguard.$$.log"
SMP="/tmp/stallguard.$$.sample"
cd "$REPO" || exit 3

echo "[stall-guard] target=$TARGET stall=${STALL_SECS}s log=$LOG"
"$ELISAC" test "$TARGET" --project . > "$LOG" 2>&1 &
RUNPID=$!

started=0; stall=0; last_size=-1; still=0
while kill -0 "$RUNPID" 2>/dev/null; do
  grep -q "\[ RUN " "$LOG" 2>/dev/null && started=1
  size=$(wc -c < "$LOG" 2>/dev/null || echo 0)
  if [ "$started" = 1 ] && [ "$size" = "$last_size" ]; then still=$((still+2)); else still=0; fi
  last_size=$size
  if [ "$started" = 1 ] && [ "$still" -ge "$STALL_SECS" ]; then stall=1; break; fi
  sleep 2
done

if [ "$stall" != 1 ]; then
  wait "$RUNPID"; rc=$?
  echo "[stall-guard] target exited rc=$rc (no stall); last lines:"
  tail -3 "$LOG"
  exit "$rc"
fi

PID=$(ps -Ao pid,args | grep "test_runners" | grep -v grep | awk 'NR==1{print $1}')
CPU=$(ps -Ao pid,pcpu | awk -v p="$PID" '$1==p{print $2}')
KIND=$(awk -v c="${CPU:-0}" 'BEGIN{print (c>50)?"BUSY-SPIN (polling a flag / busy-wait)":"BLOCKED-WAIT (parked on a kernel object)"}')
echo "=========================================================="
echo "[stall-guard] STALL: no boot progress for ${still}s."
echo "[stall-guard] runner pid=${PID:-?} cpu=${CPU:-?}%  -> ${KIND}"
echo "[stall-guard] last runtime lines:"
grep -vE "warning:|^/Users/.*\.elisa:|^ld: warning|requires can\[|^[[:space:]]*$" "$LOG" | tail -4
if [ -n "${PID:-}" ]; then
  /usr/bin/sample "$PID" 3 -file "$SMP" >/dev/null 2>&1
  echo "[stall-guard] hot stack (top-of-stack samples):"
  sed -n '/Sort by top of stack/,/Binary Images/p' "$SMP" 2>/dev/null | grep -E "Rosetta|runner|0x" | head -6
  if command -v lldb >/dev/null 2>&1; then
    echo "[stall-guard] HLE ring probe (best-effort; needs symbols):"
    lldb --batch -p "$PID" \
      -o "image lookup -s linker_hle_call_ring_index" \
      -o "image lookup -s linker_hle_call_ring" \
      -k "quit" 2>/dev/null | grep -iE "Address|Summary|linker_hle" | head -6
  fi
fi
kill -9 "$RUNPID" 2>/dev/null; pkill -9 -f "test_runners" 2>/dev/null
echo "[stall-guard] verdict: ${KIND%% *} at the guest-exec handoff. See"
echo "              verification_findings/gap3-reframe-guest-spin.md for the analysis."
echo "=========================================================="
exit 2
