#!/usr/bin/env bash
# Escape-hatch ratchet.
#
# Counts `trusted Unsafe.<Category>` sites in core/ + easm/ and fails if any
# category exceeds tools/unsafe_baseline.txt, or if a new category appears.
# This bounds erosion of the safety surface: adding an unsafe escape hatch is
# allowed, but only with a deliberate baseline bump (a visible review signal),
# not silently. Removing sites and lowering the baseline is always welcome.
#
# Usage: tools/unsafe_ratchet.sh   (exit 0 = within baseline, 1 = grew)
set -euo pipefail
cd "$(dirname "$0")/.."
BASELINE="tools/unsafe_baseline.txt"

cur="$(mktemp)"
trap 'rm -f "$cur"' EXIT
grep -rhoE 'trusted Unsafe\.[A-Za-z]+' core easm \
  | sed 's/^trusted //' | sort | uniq -c \
  | awk '{print $2" "$1}' | sort > "$cur"

fail=0

# Fail on any category that exceeds its baseline maximum.
while read -r cat base; do
  case "$cat" in ''|\#*) continue;; esac
  have="$(awk -v c="$cat" '$1==c{print $2}' "$cur")"; have="${have:-0}"
  if [ "$have" -gt "$base" ]; then
    echo "RATCHET FAIL: $cat grew ${base} -> ${have}"
    echo "  -> remove the new escape hatch, or bump $cat in $BASELINE with a rationale."
    fail=1
  fi
done < "$BASELINE"

# Fail on any new category absent from the baseline.
while read -r cat have; do
  case "$cat" in ''|\#*) continue;; esac
  if ! awk -v c="$cat" '$1==c{f=1} END{exit !f}' "$BASELINE"; then
    echo "RATCHET FAIL: new unsafe category $cat ($have) is not in $BASELINE"
    echo "  -> add it to $BASELINE with a rationale."
    fail=1
  fi
done < "$cur"

total="$(awk '{s+=$2} END{print s+0}' "$cur")"
if [ "$fail" -ne 0 ]; then
  echo "Escape-hatch ratchet FAILED (current total: ${total})."
  exit 1
fi
echo "Escape-hatch ratchet OK (${total} trusted-Unsafe sites, all within baseline)."
