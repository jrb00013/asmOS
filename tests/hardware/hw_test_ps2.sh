#!/bin/bash
# Real PS2 hardware smoke test via ps2link serial (requires PS2_IP).
set -e
PS2_IP="${PS2_IP:-}"
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"

if [ -z "$PS2_IP" ]; then
    echo "Set PS2_IP to run hardware tests"
    exit 0
fi

bash "$ROOT/scripts/build_fmcb_package.sh"
bash "$ROOT/scripts/install_fmcb.sh" "$PS2_IP"
echo "Hardware test: install complete — verify shell on PS2 console"
