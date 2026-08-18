#!/usr/bin/env bash
# Dual-universe consistency proof — x86 (QEMU surrogate) vs PS2 (native EE).
#
# This used to hash the same literal shell string twice, which trivially
# "matched" every run regardless of what either build actually contained.
# That told you nothing about HAL parity. This version inspects the two
# *real* build artifacts (build/kernel.elf for x86, build/asmos.elf for
# PS2) and diffs a real, build-derived signal: the set of shell command
# names actually linked into each binary's rodata (src/shell.c's command
# table is shared source, so both platforms should expose the same
# command surface even though their HAL backends differ underneath).
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
PROOF_DIR="$ROOT/build/proof"
mkdir -p "$PROOF_DIR"

# Command names pulled straight from the shell command table (src/shell.c),
# so this list can't silently drift out of sync with the real source.
mapfile -t CMD_NAMES < <(grep -oE '\{"[a-z0-9_]+", cmd_' src/shell.c | sed -E 's/\{"([a-z0-9_]+)".*/\1/' | sort -u)

extract_commands() {
    local elf="$1"
    local out="$2"
    : > "$out"
    # Dump the string table once (not once per command): with `pipefail`
    # set, `strings ... | grep -q ...` can report a spurious failure even
    # on a real match, because grep -q exits as soon as it finds one and
    # the resulting SIGPIPE makes `strings` itself exit non-zero, which
    # pipefail then propagates as the pipeline's status.
    local dump
    dump="$(strings -n 2 "$elf")"
    for name in "${CMD_NAMES[@]}"; do
        # GCC's -O2 constant merging can fold a short command-name literal
        # (e.g. "help") as a suffix of a longer string that already ends in
        # it (e.g. "cmd_help"), so it won't always show up as its own
        # NUL-terminated run — `strings` has no idea a pointer aims at the
        # middle of another string. A substring match on the raw string
        # table is the reliable signal here.
        if grep -qF "$name" <<< "$dump"; then
            echo "$name" >> "$out"
        fi
    done
    sort -o "$out" "$out"
}

echo "=== ASMOS dual-universe HAL parity check ==="

make -s all
if [ ! -f build/kernel.elf ]; then
    echo "FAIL: x86 build did not produce build/kernel.elf"
    exit 1
fi
extract_commands build/kernel.elf "$PROOF_DIR/x86-commands.txt"
x86_count=$(wc -l < "$PROOF_DIR/x86-commands.txt")
echo "[x86] $x86_count/${#CMD_NAMES[@]} shell commands present in kernel.elf"

if [ ! -f build/asmos.elf ]; then
    echo "SKIP: build/asmos.elf not present (no PS2SDK/ee-gcc cross-compiler" \
         "in this environment — run 'make ps2-native' with PS2DEV/PS2SDK or" \
         "ASMOS_PS2_BACKEND=docker set up to produce it)"
    echo "PARITY: x86-only — cannot compare until a PS2 build exists"
    exit 0
fi

extract_commands build/asmos.elf "$PROOF_DIR/ps2-commands.txt"
ps2_count=$(wc -l < "$PROOF_DIR/ps2-commands.txt")
echo "[ps2] $ps2_count/${#CMD_NAMES[@]} shell commands present in asmos.elf"

if diff -u "$PROOF_DIR/x86-commands.txt" "$PROOF_DIR/ps2-commands.txt"; then
    echo "PARITY: CONSISTENT — identical shell command surface on both platforms"
    exit 0
else
    echo "PARITY: DIVERGENT — see diff above (expected until full HAL parity)"
    exit 1
fi
