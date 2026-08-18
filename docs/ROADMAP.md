# ASMOS Roadmap

This describes where ASMOS actually stands after the `feat/roadmap-implementation`
and `feat/roadmap-round-2` work, and what's left. It replaces guesswork with
what was verified in this environment: an x86 GCC/NASM toolchain locally, a
real QEMU 8.2 (`qemu-system-i386`) via a throwaway `ubuntu:24.04` Docker
container (no QEMU package is installed directly in this sandbox), and a
PS2SDK/ee-gcc cross toolchain via the `ps2dev/ps2dev` Docker image
(`ASMOS_PS2_BACKEND=docker bash scripts/build_ps2_native.sh`). No real PS2
hardware or FreeMCBoot device was available to test against.

**Round 2 update: QEMU boot-to-kernel-entry is now confirmed working, for
real, not just compiled.** The previous round of this document reported the
QEMU boot investigation as inconclusive. Digging further (see "What changed
in round 2" below) found and fixed five distinct, compounding bugs in the x86
boot chain that had made it never actually boot on any BIOS/QEMU, ever, plus
one bug in how the test itself invoked QEMU. Booting `disk/os.img` in a real
QEMU now prints, in order: `DEBUG:LOADER_START`, `DEBUG:KERNEL_LOADED`,
`DEBUG:KERNEL_START` — the boot sector, stage2 loader, FAT12 kernel load, and
protected-mode handoff into the real C kernel entry point (`_kernel_start`)
all work.

## Current state (this branch)

- **Scheduler / context switch** — fixed. `task_yield_asm`/`run_scheduler_asm`
  (`boot/arch_x86/context.asm`) now save/restore EFLAGS as well as the GPRs,
  and `add_task()` (`src/scheduler.c`) builds a real initial context frame
  (zeroed GPRs + a real default EFLAGS + return address) instead of a
  "pusha placeholder." Cooperative round-robin switching between tasks
  preserves the interrupt flag across a yield.
- **PS2 reboot** — fixed. `platform/ps2/hal_system.c` now defines a real
  `system_reboot()` using PS2SDK's `LoadExecPS2("rom0:PS2LOGO", ...)`, the
  same "hand control to another ELF" call homebrew loaders use for a
  soft-reset/return-to-browser path. Previously this only printed a message
  and returned, and the PS2-native link was actually broken (`system_reboot`
  had no PS2-side definition at all — discovered while verifying this work).
- **FAT12 directories** — implemented on both platforms. The x86 HAL
  (`platform/x86/hal_storage.c`) now has a real FAT cluster allocator
  (previously every file write hardcoded cluster 2, so multi-file volumes
  would corrupt each other) plus subdirectory chain loading, `cd`/`pwd`/
  `mkdir`. The PS2 HAL (`platform/ps2/hal_storage.c`) does a real
  `opendir`/`readdir`/`stat` walk instead of returning an empty list, and
  tracks a cwd-relative memory-card path so the same shell commands work
  the same way there.
- **Network clients** — `telnet_client()` now drives an actual
  request/response loop (drain incoming, block for a line via
  `plat_read_line`, send it, drain the reply) instead of printing "type via
  future interactive mode" and closing. `ftp_client()` now supports
  `list`/`get`/`put` sub-commands that exercise real LIST/RETR/STOR exchanges
  against `src/net/udp.c`'s stream abstraction and read/write local FAT12
  files via `plat_fs_read`/`plat_fs_write`.
- **Dual-universe parity test** (`tests/dual_universe/proof.sh`) — rewritten.
  It previously hashed the same literal string for "both platforms," which
  passed unconditionally and proved nothing. It now diffs the actual shell
  command surface linked into `build/kernel.elf` and `build/asmos.elf`.

## What changed in round 2: the QEMU boot investigation

The previous round left this as an open question with a wrong guess at the
cause (assumed to be a QEMU-version/chardev-buffering quirk). Instrumenting
the actual boot chain with temporary debugcon markers at each stage — and
disassembling the real assembled bytes at each step rather than trusting the
source — found the real causes, all in `boot/bootsect.asm` / `boot/loader.asm`
/ `llinker/linker.ld` (see that commit for full detail on each):

1. `bootsect.asm`'s initial `jmp` targeted a label placed *after* its own
   entire init block, making CPU/segment setup, the drive-number store, and
   the video-mode call dead code on every boot, ever.
2. `bootsect.asm` never actually loaded the stage2 loader from disk before
   jumping to it — BIOS only loads the boot sector itself; nothing brought
   the loader into memory. Added a real INT13h read for it.
2b. `loader.asm` read the boot drive number from the wrong BPB byte offset
   (off by one from where `bootsect.asm` actually stores it).
3. `loader.asm`'s real entry point wasn't at its load address, because an
   `%include` for unrelated helper code came first in the file.
4. `loader.asm`'s FAT12 directory search compared the wrong segment for its
   own "KERNEL  BIN" string constant, so it matched arbitrary empty
   directory entries instead of the real one, and only "worked" by finding
   a directory entry with `cluster=0`.
5. `loader.asm` tried to read kernel sectors straight to the physical 1MB
   mark via real-mode INT13h (`ES:BX = 0xFFFF:0x0010`) — this reliably
   fails under QEMU/SeaBIOS (`AH=0x20`, "controller failure") even with A20
   enabled. Now stages to low memory and relocates with a 32-bit copy once
   in protected mode.
6. The final jump target (`0x100000`) was the kernel image's *load*
   address, not its *entry point* — `0x100000` holds a 4-byte magic value,
   and `_kernel_start` landed wherever the linker's default object order put
   it (confirmed via `readelf -h`: entry was `0x106c60`). Pinned
   `_kernel_start` to a fixed, predictable offset via its own linker
   section.

Plus one bug in the test itself: `tests/integration/boot_qemu.sh`'s
`-device isa-debugcon ... -debugcon file:...` invocation errors out
immediately against current QEMU (needs an explicit `-chardev`), and it was
attaching the disk via `-drive if=ide`, which makes SeaBIOS use its own
auto-detected CHS geometry instead of trusting this image's actual FAT12 BPB
(18 sectors/track, 2 heads) — every CHS disk read in the boot chain was
silently landing on the wrong physical sectors as a result. `disk/os.img` is
a floppy-formatted FAT12 volume; it needs `-fda`, not `-drive if=ide`.

## What's still open

- **HAL parity is command-surface parity, not behavioral parity.** The new
  proof.sh confirms both platforms expose the same shell command table. It
  does not (and structurally cannot, without real hardware or a PS2 emulator)
  confirm that e.g. `plat_fs_list` returns the same ordering, or that
  `plat_reboot()` actually resets a real console. That remains a real gap:
  closing it needs either PCSX2/real hardware access, or a much heavier
  simulation harness.
- **The kernel shell's actual interactive behavior past `_kernel_start` was
  not exercised beyond confirming `kernel_main()` starts running** (QEMU run
  headlessly with `-display none` and no keyboard input attached — the
  shell's `plat_read_line` presumably blocks waiting for input, which is
  expected/correct behavior for an interactive shell, not a hang). Driving
  actual shell commands (the new `cd`/`mkdir`/`ftp`/`telnet` behavior from
  round 1) through a live QEMU session — e.g. via QEMU's monitor/serial
  console fed a scripted keystroke stream — is real follow-up work.
- **FTP/telnet protocol behavior is unverified end-to-end.** The underlying
  transport (`src/net/udp.c`) is this project's own best-effort,
  non-blocking, single-packet-oriented stream — not a real TCP stack. The
  new client logic compiles and is logically correct against that API, but
  there's no real FTP/telnet server and no working network path in this
  sandbox to interop-test against.
- **`src/fat12.c` (the boot-stage2/legacy FAT12 implementation) is excluded
  from the x86 kernel build** (see the `KERNEL_C` filter in `Makefile`) and
  was not touched by this work — the live FAT12 path for the running kernel
  is entirely `platform/x86/hal_storage.c` / `platform/ps2/hal_storage.c`.
  If `src/fat12.c` is ever wired back in, it needs the same directory-nav and
  real-allocator treatment.
- **IRC client** (`irc_client()`) was left as-is: it already does a real
  NICK/USER/JOIN handshake and wasn't in the original gap list.

## Toolchain notes for future work

- x86: `gcc`, `nasm`, `ld` are present locally; QEMU is not (`make run` and
  `tests/integration/boot_qemu.sh` need `qemu-system-i386` or
  `qemu-system-x86_64` installed). It's easy to get one anyway: a throwaway
  `docker run --rm -v $PWD:/workspace -w /workspace ubuntu:24.04 bash -c
  "apt-get update -qq && apt-get install -y qemu-system-x86 && bash
  tests/integration/boot_qemu.sh"` installs a real QEMU 8.2 and runs the
  boot test against it — this is exactly how the round-2 boot-chain fixes
  were verified here, and it's the recommended way to check any future
  x86 boot-path change in this environment.
- PS2: no PS2SDK/ee-gcc is installed directly in this environment, but
  `scripts/build_ps2_native.sh` with `ASMOS_PS2_BACKEND=docker` successfully
  pulls and uses the `ps2dev/ps2dev:latest` Docker image to produce a real
  `build/asmos.elf`. That's the recommended way to verify PS2-side changes
  compile here going forward — it caught the missing `system_reboot()`
  linker error during this work that the x86-only build could never have
  caught.
