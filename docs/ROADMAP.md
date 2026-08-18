# ASMOS Roadmap

This describes where ASMOS actually stands after the `feat/roadmap-implementation`
work, and what's left. It replaces guesswork with what was verified in this
environment: an x86 GCC/NASM/QEMU toolchain locally, plus a PS2SDK/ee-gcc cross
toolchain via the `ps2dev/ps2dev` Docker image (`ASMOS_PS2_BACKEND=docker bash
scripts/build_ps2_native.sh`). No real PS2 hardware, FreeMCBoot device, or a
working QEMU boot-to-shell session was available to test against, so anything
marked "unverified" below means "compiles/links, behavior not observed running."

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

## What's still open

- **HAL parity is command-surface parity, not behavioral parity.** The new
  proof.sh confirms both platforms expose the same shell command table. It
  does not (and structurally cannot, without real hardware or a PS2 emulator)
  confirm that e.g. `plat_fs_list` returns the same ordering, or that
  `plat_reboot()` actually resets a real console. That remains a real gap:
  closing it needs either PCSX2/real hardware access, or a much heavier
  simulation harness.
- **QEMU boot-to-shell was not confirmed working in this environment.**
  `tests/integration/boot_qemu.sh`'s `-device isa-debugcon ... -debugcon
  file:...` invocation errors immediately against a current (8.2) QEMU (the
  chardev has to be created explicitly with newer QEMU — `-chardev
  file,id=dbg,path=... -device isa-debugcon,iobase=0xe9,chardev=dbg`). Fixing
  that let QEMU run without erroring, but the debug markers
  (`DEBUG:LOADER_START`/`DEBUG:KERNEL_LOADED`/`DEBUG:KERNEL_START`) still did
  not appear in the capture within the time investigated. Whether that's a
  qemu-version/BIOS quirk, a boot-sector/loader issue, or a debugcon
  chardev-buffering artifact was not conclusively isolated — this needs
  follow-up with either an older QEMU build or a serial/VGA-based capture
  method instead of isa-debugcon. `make test-integration`'s own QEMU step
  still correctly SKIPs when qemu-system-x86 isn't installed, which is the
  common case in this environment.
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
  `qemu-system-x86_64` installed — see the note above about the `-debugcon`
  invocation needing an explicit `-chardev` on current QEMU).
- PS2: no PS2SDK/ee-gcc is installed directly in this environment, but
  `scripts/build_ps2_native.sh` with `ASMOS_PS2_BACKEND=docker` successfully
  pulls and uses the `ps2dev/ps2dev:latest` Docker image to produce a real
  `build/asmos.elf`. That's the recommended way to verify PS2-side changes
  compile here going forward — it caught the missing `system_reboot()`
  linker error during this work that the x86-only build could never have
  caught.
