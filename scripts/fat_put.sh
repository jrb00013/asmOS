#!/usr/bin/env bash
# Install one or more files into FAT12 image with proper cluster chains.
set -euo pipefail
IMG="${1:?image}"
shift
if [[ $# -lt 2 ]]; then
    echo "usage: fat_put.sh image file NAME83 [file NAME83 ...]" >&2
    exit 1
fi
python3 "$(dirname "$0")/../tools/fat_put.py" "$IMG" "$@"
