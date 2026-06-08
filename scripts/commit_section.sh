#!/usr/bin/env bash
# Stage and commit with a message. Usage: commit_section.sh "message" file1 file2 ...
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
MSG="${1:?commit message required}"
shift
if [[ $# -eq 0 ]]; then
    echo "No files specified" >&2
    exit 1
fi
git add "$@"
if git diff --cached --quiet; then
    echo "[skip] nothing to commit for: $MSG"
    exit 0
fi
git commit -m "$(cat <<EOF
$MSG
EOF
)"
echo "[commit] $MSG"
