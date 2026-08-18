# Implementation Plan

Ordered, commit-sized steps for the work delivered on `feat/roadmap-implementation`.
Each step was built and checked before moving to the next (`make all`,
`make test-integration`, and — once available — a PS2-native Docker cross
build via `ASMOS_PS2_BACKEND=docker bash scripts/build_ps2_native.sh`).

1. **Scheduler context switch (highest priority — real correctness bug).**
   `boot/arch_x86/context.asm`: add `pushfd`/`popfd` around the existing
   `pusha`/`popa` in `task_yield_asm` and `run_scheduler_asm`, so EFLAGS
   (interrupt flag included) survives a cooperative yield. `src/scheduler.c`:
   change `add_task()`'s stack setup from an ad hoc "8 zeroed words + return
   address" layout to a frame that mirrors exactly what the new save path
   produces (8 GPRs + EFLAGS + return address), so first-run and resumed
   tasks share one restore path. Verify: `make all`, `make test-integration`.
   → commit `552f655`.

2. **FAT12 directory support (second priority).**
   `platform/x86/hal_storage.c`: add a real FAT12 cluster allocator
   (`fat_alloc_cluster`/`fat_free_chain`/`fat_set_next_cluster`/
   `fat_flush_fat`) to replace the previous hardcoded-cluster-2 write path;
   add current-directory tracking (`cur_dir_cluster`/`cur_dir_parent`/
   `cwd_path`) and subdirectory cluster-chain loading so find/list/read/
   write/delete all operate against whichever directory is current; add
   `plat_fs_chdir`/`plat_fs_mkdir`/`plat_fs_cwd` with real `.`/`..` entries.
   `platform/ps2/hal_storage.c`: replace the empty-stub `plat_fs_list` with
   a real `opendir`/`readdir`/`stat` walk; add matching chdir/mkdir/cwd
   tracking a cwd-relative memory-card path. `include/platform.h`: declare
   the three new functions. `src/shell.c`: wire `cd`/`cat` to the new calls
   instead of printing "(nav not impl)"/"(read not impl)"; add `pwd` and
   `mkdir` commands. Verify: `make all`, `make test-integration`, and a
   PS2-native Docker cross build. → commit `41b8141`.

3. **PS2 reboot (third priority).**
   `platform/ps2/hal_system.c`: replace the print-only `plat_reboot()` with
   a real `system_reboot()` using PS2SDK's `LoadExecPS2("rom0:PS2LOGO", ...)`.
   This step is what first surfaced, via the PS2-native Docker build, that
   `system_reboot` had no PS2-side definition at all — `src/kernel.c`'s
   shared `halt_system()` calls it directly and the link was actually
   broken before this fix. Also tightened truncation-prone `snprintf`
   buffer sizes in `platform/ps2/hal_storage.c` found while getting this to
   link cleanly. Verify: full PS2-native ELF link succeeds; x86 `make all`/
   `make test-integration` unaffected. → commit `ef540e9`.

4. **Net client loops (fourth priority).**
   Rewrite `src/net_clients.c`: `telnet_client()` gets a real
   request/response loop (drain incoming, block for one line of local
   input, send it, drain the reply, `/quit` to exit) instead of a one-shot
   banner print; `ftp_client()` gets `list`/`get <remote>`/`put <local>`
   sub-commands that do a genuine LIST/RETR/STOR exchange over the
   existing `net_stream_*` API and move bytes through `plat_fs_read`/
   `plat_fs_write`. Verify: x86 `make all` and a PS2-native Docker cross
   build both compile/link with no errors; `make test-integration` passes.
   → commit `4ab7c14`.

5. **HAL parity test work (fifth priority).**
   Rewrite `tests/dual_universe/proof.sh` so it inspects the real build
   artifacts (`build/kernel.elf`, `build/asmos.elf`) instead of hashing an
   identical literal string for both "platforms." It now diffs the set of
   shell command names (from `src/shell.c`'s command table) actually linked
   into each binary, and reports SKIP honestly when no PS2 build exists
   instead of claiming consistency. Also fixed a `pipefail`/SIGPIPE bug
   discovered while building it (`strings | grep -q` can report failure on
   a real match under `pipefail`). Verify: ran end-to-end with real x86 and
   PS2-native artifacts — 47/47 commands present, "PARITY: CONSISTENT".
   → commit `5088c11`.

6. **Small real cleanups surfaced along the way.**
   `src/net/udp.c`: split the `if (...) return -1; p++;` one-liners in
   `net_parse_ip` onto separate lines (GCC's own
   `-Wmisleading-indentation` warning was correct that this reads wrong),
   and drop an unused `seg` variable in `net_ip_to_str`. Verify: `make all`
   builds warning-clean for this file; `make test-integration` passes.
   → commit `5e5db29`.

7. **Documentation.** `docs/ROADMAP.md` (current state + open gaps) and this
   file. → commits `3120299` and this one.

## What was deliberately *not* done

- **`src/fat12.c`** (excluded from the x86 `KERNEL_C` build via the
  `Makefile` filter) was left untouched. It's not on the live code path for
  either platform's running kernel; touching it would have been scope
  creep against a file nothing currently links.
- **True behavioral HAL parity** (not just command-surface parity) is
  explicitly out of scope for what could be verified here — it needs real
  PS2 hardware or PCSX2, neither of which is available in this environment.

## Round 2: `feat/roadmap-round-2` (branched from `feat/roadmap-implementation`)

Round 1 called the QEMU boot investigation inconclusive and guessed the
cause was a QEMU-version/chardev-buffering quirk. Asked to dig further and
get a real boot-to-kernel-entry proof if at all possible, the actual root
causes turned up instead: five distinct, compounding bugs in the x86 boot
chain itself that had made it never boot on any BIOS/QEMU, ever (not a
QEMU-version issue at all), plus one bug in how the test invoked QEMU.

1. **Root-cause via instrumentation.** Added temporary debugcon (`out
   0xE9`) markers at each stage of `boot/bootsect.asm`/`boot/loader.asm`
   and disassembled the actual assembled bytes (`objdump -D -b binary -m
   i8086 --adjust-vma=0x7c00 ...`) at each step, rather than trusting the
   source. This is what actually found each bug below — none of them were
   visible from reading the code alone. → commit `be3d68c` (fixes folded
   in together; the instrumentation itself was removed before committing).
2. **Fix the boot chain (six bugs, one commit since they're compounding —
   fixing any one alone still wouldn't boot):** dead init code from a
   mislabeled jump, no disk read for the stage2 loader at all, an off-by-
   one BPB drive-number offset, a loader entry point buried behind
   unrelated included code, a wrong-segment string compare that "found"
   empty directory entries instead of KERNEL.BIN, an unreliable >=1MB
   real-mode disk read, and a hardcoded kernel jump target that pointed at
   a magic-number byte instead of the real entry point. Verify: real QEMU
   8.2 boot (via a throwaway `ubuntu:24.04` Docker container, since no
   QEMU is installed directly in this sandbox) prints
   `DEBUG:LOADER_START` / `DEBUG:KERNEL_LOADED` / `DEBUG:KERNEL_START`.
   → commit `be3d68c`.
3. **Fix the test's own QEMU invocation.** `-debugcon file:...` needs an
   explicit `-chardev` on current QEMU, and the disk needs `-fda` (it's a
   floppy-formatted FAT12 volume) instead of `-drive if=ide` (which made
   SeaBIOS use its own auto-detected geometry instead of trusting the
   BPB — the underlying reason several of the boot-chain bugs were even
   reachable to hit). Verify: `bash tests/integration/boot_qemu.sh` prints
   `PASS: QEMU boot reached kernel entry` against a real QEMU. → commit
   `4bfd602`.
4. **Fix `ASMOS.MET`'s recorded entry point** to match the real,
   now-fixed entry address instead of the load address — a correctness
   follow-through from bug 6 above, even though nothing currently reads
   this field back. Verify: `make all` / `make test-integration` pass.
   → commit `4d9c8e4`.
5. **Documentation.** Updated `docs/ROADMAP.md` with what round 2 actually
   found (superseding round 1's wrong guess) and how to get a real QEMU
   in this sandbox for future verification. → commit `c1bbdc0`.
6. **Fix `make run`** — it had the exact same `-drive if=ide` bug as the
   test script; the actual developer-facing "boot this locally" command
   needed the same floppy-geometry fix. Verify: ran `make run` against a
   real QEMU in a throwaway container — disk build and QEMU invocation
   both succeed; the only failure there is GTK display init in a headless
   container, unrelated to the boot path itself (confirmed separately via
   the `-display none` invocation used for verification, which reaches
   real `DEBUG:KERNEL_START` output). → commit `ce3f0cf`.

### What's still open after round 2

- **Interactive shell behavior past kernel entry is unexercised.** QEMU was
  only run headlessly (`-display none`, no keyboard/serial input attached)
  to capture debugcon markers. Confirming `kernel_main()`'s shell actually
  responds to commands (the round-1 `cd`/`mkdir`/`ftp`/`telnet` work) would
  need a scripted keystroke stream fed through QEMU's monitor or serial
  console — a bounded but separate piece of follow-up work.
- Everything else called out as open in `docs/ROADMAP.md` (behavioral HAL
  parity, FTP/telnet protocol interop, `src/fat12.c`) remains open for the
  same reasons stated there.
