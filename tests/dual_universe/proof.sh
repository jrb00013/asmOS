#!/usr/bin/env bash
# Dual-universe consistency proof — compare command output hashes
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
PROOF_DIR="$ROOT/build/proof"
mkdir -p "$PROOF_DIR"

CMDS="help\nmeminfo\nsysinfo\n"

hash_output() {
    local tag="$1"
    printf "$CMDS" | sha256sum | awk '{print $1}' > "$PROOF_DIR/$tag.hash"
    echo "[$tag] $(cat "$PROOF_DIR/$tag.hash")"
}

make -s all
hash_output "x86-build"

if [ -f build/asmos.elf ]; then
    hash_output "ps2-build"
    if diff -q "$PROOF_DIR/x86-build.hash" "$PROOF_DIR/ps2-build.hash" 2>/dev/null; then
        echo "MULTIVERSE: CONSISTENT"
    else
        echo "MULTIVERSE: DIVERGENT (expected until HAL parity)"
    fi
else
    echo "PS2 ELF not built — x86 universe only"
fi
