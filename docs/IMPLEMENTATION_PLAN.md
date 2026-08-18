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
- **`tests/integration/boot_qemu.sh`** was not rewritten as part of this
  plan, even though investigating it (see `docs/ROADMAP.md`) turned up a
  real `-debugcon`/`-chardev` compatibility issue with current QEMU. Fixing
  the debug-marker capture mechanism is its own bounded piece of work and
  wasn't one of the prioritized gaps; it's called out as follow-up instead
  of being fixed speculatively without being able to confirm the fix
  actually restores marker capture.
- **True behavioral HAL parity** (not just command-surface parity) is
  explicitly out of scope for what could be verified here — it needs real
  PS2 hardware or PCSX2, neither of which is available in this environment.
