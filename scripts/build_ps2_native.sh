#!/usr/bin/env bash
# Build ASMOS PS2 native ELF — uses ASMOS_PS2_BACKEND from env/asmos.env when set.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

# Load setup selection if present
if [[ -f "$ROOT/env/asmos.env" ]]; then
    # shellcheck source=/dev/null
    source "$ROOT/env/asmos.env"
fi

PS2DEV="${PS2DEV:-$HOME/ps2dev}"
PS2SDK="${PS2SDK:-$PS2DEV/ps2sdk}"
DOCKER_IMAGE="${ASMOS_PS2DOCKER:-ps2dev/ps2dev:latest}"
BACKEND="${ASMOS_PS2_BACKEND:-auto}"

build_local() {
    [[ -f "$PS2SDK/samples/Makefile.eeglobal" ]] || return 1
    echo "[ps2-native] backend=native PS2SDK=$PS2SDK"
    export PS2DEV PS2SDK
    export PATH="$PS2DEV/bin:$PS2DEV/ee/bin:$PS2DEV/iop/bin:$PS2SDK/bin:$PATH"
    make -C platform/ps2 clean
    make -C platform/ps2
}

build_docker() {
    command -v docker >/dev/null 2>&1 || return 1
    echo "[ps2-native] backend=docker image=$DOCKER_IMAGE"
    docker run --rm \
        -v "$ROOT:/workspace" \
        -w /workspace \
        -e PS2DEV=/usr/local/ps2dev \
        -e PS2SDK=/usr/local/ps2dev/ps2sdk \
        "$DOCKER_IMAGE" \
        bash -lc 'export PATH=$PS2DEV/bin:$PS2DEV/ee/bin:$PS2DEV/iop/bin:$PS2SDK/bin:$PATH && make -C platform/ps2 clean && make -C platform/ps2'
}

pick_backend() {
    case "$BACKEND" in
        native)
            echo native ;;
        docker)
            echo docker ;;
        none)
            echo none ;;
        auto|*)
            if [[ -f "$PS2SDK/samples/Makefile.eeglobal" ]] || command -v ee-gcc >/dev/null 2>&1; then
                echo native
            elif command -v docker >/dev/null 2>&1 && docker image inspect "$DOCKER_IMAGE" >/dev/null 2>&1; then
                echo docker
            else
                echo none
            fi
            ;;
    esac
}

SELECTED="$(pick_backend)"
echo "[ps2-native] selected backend: $SELECTED (ASMOS_PS2_BACKEND=${BACKEND})"

case "$SELECTED" in
    native)
        build_local
        ;;
    docker)
        build_docker
        ;;
    none)
        echo "No PS2 backend available. Run: ./setup.sh" >&2
        exit 1
        ;;
esac
