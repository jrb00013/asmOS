#!/usr/bin/env bash
# Sectional commits for v3.0 — runs commit_section.sh per unit.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
C=scripts/commit_section.sh
chmod +x "$C" scripts/fat_put.sh tests/dual_universe/proof.sh 2>/dev/null || true

run() { "$C" "$1" "${@:2}"; }

# Boot layer
run "Add ASMOS boot ABI header (ASMOSBT1, load addr 0x100000)" include/boot_meta.h
run "Add block device interface for sector I/O" include/block_dev.h
run "Add unified FAT12 layout constants and API" include/fat12.h
run "Add boot sector BPB and jump to stage2 loader" boot/bootsect.asm
run "Add stage2 FAT12 KERNEL.BIN loader with protected mode" boot/loader.asm
run "Add gen_boot_meta tool for ASMOS.META CRC descriptor" tools/gen_boot_meta.c
run "Add kernel entry trampoline with ASMK magic" src/kernel_start.c
run "Relink kernel at 0x100000 with boot signature section" llinker/linker.ld

# Storage core
run "Add BIOS block device implementation (boot use)" src/block_dev.c
run "Add unified FAT12 cluster walk implementation" src/fat12.c
run "Add FAT12 install script without mtools dependency" scripts/fat_put.sh
run "Align x86 HAL storage FAT layout to standard BPB" platform/x86/hal_storage.c

# Subsystems
run "Add subsystem registry header and capability flags" include/subsys.h
run "Add subsystem registry with init/tick/status hooks" src/subsys/registry.c
run "Wire kernel v3 init through subsystem registry" src/kernel.c

# Quantum / novel
run "Add quantum subsystem public API" include/quantum.h
run "Add observer RAM collapse for subsystems" src/quantum/observer.c
run "Add Kuramoto-style phase sync transport tick" src/quantum/phase_sync.c
run "Add entangled controller port bindings" src/quantum/entangle.c
run "Add eigenmode audio mode mixer stubs" src/quantum/eigenmode.c
run "Add wavefunction config loader stub" src/quantum/config_wf.c
run "Add CONFIG.WF default wavefunction profiles" boot/CONFIG.WF

# Network
run "Declare ICMP ping in network stack API" include/net.h
run "Add minimal ICMP echo implementation" src/net/icmp.c
run "Prefer ICMP then ASM framing in plat_net_ping" platform/x86/hal_net.c

# Shell
run "Add quantum REPL commands and subsys tick loop" src/shell.c

# PS2
run "Add PS2 IOP module loader skeleton" platform/ps2/iop_init.c
run "Extend PS2 Makefile for subsys quantum and icmp" platform/ps2/Makefile

# Build / test
run "Revamp Makefile for v3 boot pipeline and FAT put" Makefile
run "Add sectional commit helper script" scripts/commit_section.sh
run "Add dual-universe consistency proof script" tests/dual_universe/proof.sh

# Docs v3 sections (batched from docs/v3/)
if [ -d docs/v3 ]; then
  for f in docs/v3/*.md; do
    [ -f "$f" ] || continue
    run "Add architecture doc $(basename "$f")" "$f"
  done
fi

echo "commit_wave done: $(git rev-list --count HEAD) total commits"
