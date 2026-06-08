#!/usr/bin/env bash
# ASMOS full development environment setup
# Detects what is already installed, selects the right PS2 backend,
# and only installs missing dependencies.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
DIM='\033[0;2m'
NC='\033[0m'

USE_SUDO=1
INSTALL_DOCKER=1
INSTALL_PS2SDK_NATIVE=0
SKIP_VERIFY=0
PS2DEV_DIR="${PS2DEV:-$HOME/ps2dev}"
PS2_BACKEND="auto"   # auto | native | docker | none

usage() {
    cat <<'EOF'
ASMOS setup.sh — detect, select, and install only what you need

Usage:
  ./setup.sh [options]

Options:
  --no-sudo              Do not use sudo (install packages yourself)
  --skip-docker          Do not install or use Docker for PS2SDK
  --install-ps2sdk       Force native PS2SDK build at ~/ps2dev (~30+ min)
  --ps2-backend <mode>   Force PS2 path: auto, native, docker, none
  --ps2dev-dir <path>    PS2DEV location (default: ~/ps2dev)
  --skip-verify          Skip build verification at the end
  -h, --help             Show this help

Selection logic (default --ps2-backend auto):
  1. Native PS2SDK if ee-gcc / Makefile.eeglobal already present
  2. Else Docker ps2dev image if docker is available
  3. Else install Docker (unless --skip-docker)
  4. Else skip PS2 native (x86 build still works)

After setup:
  source env/asmos.env
  make all / make ps2-native / make fmcb-package
EOF
}

log()  { echo -e "${BLUE}[setup]${NC} $*"; }
ok()   { echo -e "${GREEN}[ok]${NC} $*"; }
warn() { echo -e "${YELLOW}[warn]${NC} $*"; }
fail() { echo -e "${RED}[fail]${NC} $*"; exit 1; }
skip() { echo -e "${DIM}[skip]${NC} $*"; }

have_cmd() { command -v "$1" >/dev/null 2>&1; }

while [[ $# -gt 0 ]]; do
    case "$1" in
        --no-sudo) USE_SUDO=0 ;;
        --skip-docker) INSTALL_DOCKER=0 ;;
        --install-ps2sdk) INSTALL_PS2SDK_NATIVE=1 ;;
        --ps2-backend) shift; PS2_BACKEND="${1:?mode required}" ;;
        --ps2dev-dir) shift; PS2DEV_DIR="${1:?path required}" ;;
        --skip-verify) SKIP_VERIFY=1 ;;
        -h|--help) usage; exit 0 ;;
        *) fail "Unknown option: $1 (try --help)" ;;
    esac
    shift
done

case "$PS2_BACKEND" in
    auto|native|docker|none) ;;
    *) fail "Invalid --ps2-backend: $PS2_BACKEND" ;;
esac

SUDO=""
if [[ "$USE_SUDO" -eq 1 ]] && [[ "$(id -u)" -ne 0 ]]; then
    if have_cmd sudo; then SUDO="sudo"; else USE_SUDO=0; fi
fi

detect_pkg_manager() {
    if have_cmd apt-get; then echo apt; return; fi
    if have_cmd dnf; then echo dnf; return; fi
    if have_cmd yum; then echo yum; return; fi
    if have_cmd pacman; then echo pacman; return; fi
    if have_cmd brew; then echo brew; return; fi
    echo unknown
}

PKG_MGR="$(detect_pkg_manager)"

# --- requirement checks ---
need_x86=0
need_docker=0
need_ps2sdk_native=0
need_ps2_image=0

check_x86_requirements() {
    local missing_cmds=()
    for c in make nasm gcc ld objcopy dd; do
        have_cmd "$c" || missing_cmds+=("$c")
    done
    if [[ ${#missing_cmds[@]} -gt 0 ]]; then
        need_x86=1
        echo "x86-missing-cmds=${missing_cmds[*]}"
    else
        echo "x86=ready"
    fi
}

have_ps2sdk_native() {
    [[ -f "$PS2DEV_DIR/ps2sdk/samples/Makefile.eeglobal" ]] && return 0
    [[ -f "${PS2DEV:-}/ps2sdk/samples/Makefile.eeglobal" ]] && return 0
    have_cmd ee-gcc && return 0
    return 1
}

have_ps2docker_image() {
    have_cmd docker && docker image inspect ps2dev/ps2dev:latest >/dev/null 2>&1
}

select_ps2_backend() {
    local chosen="$PS2_BACKEND"

    if [[ "$chosen" == "auto" ]]; then
        if have_ps2sdk_native; then
            chosen="native"
        elif have_ps2docker_image; then
            chosen="docker"
        elif [[ "$INSTALL_PS2SDK_NATIVE" -eq 1 ]]; then
            chosen="native"
        elif [[ "$INSTALL_DOCKER" -eq 1 ]]; then
            chosen="docker"
        else
            chosen="none"
        fi
    fi

    case "$chosen" in
        native)
            if ! have_ps2sdk_native; then
                need_ps2sdk_native=1
            fi
            ;;
        docker)
            if ! have_cmd docker; then
                need_docker=1
            fi
            if ! have_ps2docker_image; then
                need_ps2_image=1
            fi
            ;;
        none)
            ;;
    esac

    echo "$chosen"
}

print_plan() {
    local ps2_sel="$1"
    echo ""
    echo -e "${CYAN}=== Setup plan ===${NC}"
    echo -e "  Package manager: ${PKG_MGR}"
    echo -e "  PS2 backend:     ${ps2_sel}"
    echo -e "  PS2DEV dir:      ${PS2DEV_DIR}"
    echo ""
    if [[ "$need_x86" -eq 1 ]]; then
        echo -e "  ${YELLOW}INSTALL${NC} x86 build packages (missing tools detected)"
    else
        echo -e "  ${GREEN}SKIP${NC}    x86 packages (tools already present)"
    fi
    if [[ "$need_docker" -eq 1 ]]; then
        echo -e "  ${YELLOW}INSTALL${NC} Docker"
    elif [[ "$ps2_sel" == "docker" ]]; then
        echo -e "  ${GREEN}SKIP${NC}    Docker (already installed)"
    fi
    if [[ "$need_ps2_image" -eq 1 ]]; then
        echo -e "  ${YELLOW}PULL${NC}    ps2dev/ps2dev:latest Docker image"
    elif [[ "$ps2_sel" == "docker" ]]; then
        echo -e "  ${GREEN}SKIP${NC}    ps2dev image (already present)"
    fi
    if [[ "$need_ps2sdk_native" -eq 1 ]]; then
        echo -e "  ${YELLOW}BUILD${NC}   native PS2SDK at ${PS2DEV_DIR}"
    elif [[ "$ps2_sel" == "native" ]]; then
        echo -e "  ${GREEN}SKIP${NC}    native PS2SDK (already present)"
    fi
    if [[ "$SKIP_VERIFY" -eq 1 ]]; then
        echo -e "  ${DIM}SKIP${NC}    build verification (--skip-verify)"
    else
        echo -e "  ${YELLOW}RUN${NC}     verify: make all"
        if [[ "$ps2_sel" != "none" ]]; then
            echo -e "  ${YELLOW}RUN${NC}     verify: make ps2-native"
        fi
    fi
    echo ""
}

apt_pkg_installed() {
    dpkg -s "$1" >/dev/null 2>&1
}

install_missing_apt_packages() {
    [[ "$need_x86" -eq 1 || "$need_docker" -eq 1 ]] || return 0
    [[ "$PKG_MGR" == "apt" ]] || return 0

    local pkgs=()
    if [[ "$need_x86" -eq 1 ]]; then
        for p in build-essential nasm gcc g++ make binutils gcc-multilib g++-multilib \
                 dosfstools genisoimage dvd+rw-tools qemu-system-x86 qemu-system-x86-64 \
                 git curl ca-certificates file bsdextrautils python3; do
            apt_pkg_installed "$p" || pkgs+=("$p")
        done
    fi
    if [[ "$need_docker" -eq 1 ]]; then
        for p in docker.io; do
            apt_pkg_installed "$p" || pkgs+=("$p")
        done
    fi

    if [[ ${#pkgs[@]} -eq 0 ]]; then
        skip "All apt packages already installed"
        return 0
    fi

    log "Installing ${#pkgs[@]} missing apt package(s): ${pkgs[*]}"
    $SUDO apt-get update -qq
    $SUDO apt-get install -y "${pkgs[@]}"
}

install_missing_packages() {
    if [[ "$need_x86" -eq 0 && "$need_docker" -eq 0 ]]; then
        skip "System packages — nothing to install"
        return 0
    fi

    case "$PKG_MGR" in
        apt)
            install_missing_apt_packages
            ;;
        dnf|yum)
            log "Installing packages via $PKG_MGR..."
            local pkgs=(make nasm gcc gcc-c++ binutils dosfstools genisoimage dvd+rw-tools qemu-system-x86 git curl file)
            [[ "$need_docker" -eq 1 ]] && pkgs+=(docker)
            $SUDO "$PKG_MGR" install -y "${pkgs[@]}"
            ;;
        pacman)
            log "Installing packages via pacman..."
            $SUDO pacman -Sy --noconfirm --needed base-devel nasm gcc dosfstools cdrtools dvd+rw-tools qemu-system-x86 git curl file
            [[ "$need_docker" -eq 1 ]] && $SUDO pacman -S --noconfirm --needed docker
            ;;
        brew)
            [[ "$need_x86" -eq 1 ]] && brew install nasm gcc make binutils qemu dosfstools cdrtools git
            ;;
        *)
            warn "Install manually: nasm gcc-multilib make qemu dosfstools genisoimage"
            ;;
    esac
}

ensure_symlinks() {
    if have_cmd qemu-system-x86_64 && ! have_cmd qemu-system-i386; then
        $SUDO ln -sf "$(command -v qemu-system-x86_64)" /usr/local/bin/qemu-system-i386 2>/dev/null || true
        ok "symlink qemu-system-i386"
    fi
    if have_cmd genisoimage && ! have_cmd mkisofs; then
        $SUDO ln -sf "$(command -v genisoimage)" /usr/local/bin/mkisofs 2>/dev/null || true
        ok "symlink mkisofs"
    fi
    if have_cmd mkdosfs && ! have_cmd mkfs.fat; then
        $SUDO ln -sf "$(command -v mkdosfs)" /usr/local/bin/mkfs.fat 2>/dev/null || true
    fi
}

install_ps2sdk_native() {
    [[ "$need_ps2sdk_native" -eq 1 ]] || return 0
    log "Building native PS2SDK at $PS2DEV_DIR..."
    have_cmd git || fail "git required"
    mkdir -p "$(dirname "$PS2DEV_DIR")"
    if [[ ! -d "$PS2DEV_DIR/ps2dev" ]]; then
        git clone https://github.com/ps2dev/ps2dev.git "$PS2DEV_DIR/ps2dev"
    fi
    export PS2DEV="$PS2DEV_DIR"
    export PATH="$PS2DEV/bin:$PS2DEV/ee/bin:$PS2DEV/iop/bin:$PATH"
    bash "$PS2DEV_DIR/ps2dev/build-all.sh"
    ok "Native PS2SDK at $PS2DEV_DIR"
}

pull_ps2dev_image() {
    [[ "$need_ps2_image" -eq 1 ]] || return 0
    have_cmd docker || return 0
    log "Pulling ps2dev/ps2dev:latest..."
    docker pull ps2dev/ps2dev:latest
    ok "Docker image ready"
}

write_env_file() {
    local ps2_sel="$1"
    mkdir -p "$ROOT/env"
    local envfile="$ROOT/env/asmos.env"
    cat > "$envfile" <<EOF
# ASMOS environment — source with: source env/asmos.env
# Generated by setup.sh on $(date -Iseconds 2>/dev/null || date)
export ASMOS_ROOT="$ROOT"
export ASMOS_PS2_BACKEND="$ps2_sel"
export PATH="/usr/local/bin:\$PATH"

export PS2DEV="${PS2DEV_DIR}"
export PS2SDK="\${PS2DEV}/ps2sdk"
if [[ -d "\$PS2DEV/bin" ]]; then
  export PATH="\$PS2DEV/bin:\$PS2DEV/ee/bin:\$PS2DEV/iop/bin:\$PS2DEV/dvp/bin:\$PS2SDK/bin:\$PATH"
fi

alias asmos-build='make -C "\$ASMOS_ROOT" all'
alias asmos-qemu='make -C "\$ASMOS_ROOT" run'
alias asmos-ps2='make -C "\$ASMOS_ROOT" ps2-native'
alias asmos-fmcb='make -C "\$ASMOS_ROOT" fmcb-package'
EOF
    ok "Wrote $envfile (ASMOS_PS2_BACKEND=$ps2_sel)"
}

chmod_scripts() {
    chmod +x setup.sh install_tools.sh test_build.sh test_suite.sh test_clean_build.sh build_ps2.sh build_ultimate.sh 2>/dev/null || true
    chmod +x scripts/*.sh tests/integration/*.sh tests/hardware/*.sh 2>/dev/null || true
}

verify_tools() {
    log "Verifying required tools..."
    local missing=()
    for t in make nasm gcc ld objcopy dd; do
        have_cmd "$t" && ok "$t" || missing+=("$t")
    done
    [[ ${#missing[@]} -eq 0 ]] || fail "Still missing: ${missing[*]}"
}

verify_build() {
    [[ "$SKIP_VERIFY" -eq 1 ]] && return 0
    log "Verifying x86 build..."
    make clean >/dev/null 2>&1 || true
    make all
    ok "x86 build OK"
    [[ -x tests/integration/run_checks.sh ]] && bash tests/integration/run_checks.sh
}

verify_ps2_native() {
    [[ "$SKIP_VERIFY" -eq 1 ]] && return 0
    [[ "$SELECTED_PS2" == "none" ]] && return 0
    log "Verifying PS2 native build..."
    if make ps2-native; then
        ok "PS2 native ELF: build/asmos.elf"
    else
        warn "PS2 native build failed (x86 path still works)"
    fi
}

main() {
    echo -e "${CYAN}ASMOS setup — detect and install only what's needed${NC}"

    check_x86_requirements >/dev/null
    if [[ "$(check_x86_requirements)" == x86=ready ]]; then
        need_x86=0
    else
        need_x86=1
    fi

    SELECTED_PS2="$(select_ps2_backend)"
    print_plan "$SELECTED_PS2"

    install_missing_packages
    ensure_symlinks
    if [[ "$need_docker" -eq 1 ]]; then
        $SUDO systemctl enable --now docker 2>/dev/null || true
    fi
    install_ps2sdk_native
    pull_ps2dev_image
    chmod_scripts
    write_env_file "$SELECTED_PS2"
    verify_tools
    verify_build
    verify_ps2_native

    echo ""
    echo -e "${GREEN}=== Setup complete ===${NC}"
    echo -e "  PS2 backend selected: ${CYAN}${SELECTED_PS2}${NC}"
    echo ""
    echo "  source env/asmos.env"
    echo "  make all              # x86 OS image"
    echo "  make ps2-native       # PS2 ELF (uses \$ASMOS_PS2_BACKEND)"
    echo "  make fmcb-package     # deploy/fmcb/"
    echo ""
}

main "$@"
