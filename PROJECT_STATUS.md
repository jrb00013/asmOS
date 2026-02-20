# Project Status and GUI Plan

## Current Status

### What the project is
- **asmos**: x86 real-mode / 32-bit OS targeting PlayStation 2 (PS2) with modchip. Build produces a disk image that can run in QEMU or (in theory) on PS2 hardware.
- **Stack**: Bootloader (16-bit asm), kernel (32-bit C), shell, minimal FS (FAT12), and a graphics/game layer that is currently PS2-oriented and not active on PC/QEMU.

### What works today
- **Build**: `make all` or `make ps2-build` produces `disk/os.img`. `make run` boots it in QEMU.
- **Boot**: Bootloader loads kernel from disk and jumps to it. Kernel uses VGA text mode (80x25) at 0xB8000.
- **Shell**: Text shell with many commands (help, ls, meminfo, ps2info, clear, reboot, graphics, demo, game, etc.). Command/args parsing supports `game 1`, `demo`, etc.
- **Kernel**: Memory manager, scheduler, FAT12 init, boot splash (text-mode box drawing), kprint/kprintf.
- **GUI on PC/QEMU**: On non-PS2 builds, VGA mode 13h (320x200, 256 colors) is used. `demo` switches to mode 13h, runs the bouncing-rectangles demo, then restores text mode. `game` lists games and `game <number>` launches a game (Snake, Pong, Tetris, Space Invaders, Racing) in graphics mode with keyboard input; then restores text mode.
- **Graphics layer**: `graphics.c` uses a framebuffer abstraction (`video.h`): on PC framebuffer at 0xA0000 (mode 13h), on PS2 VRAM at 0x70000000. Integer Bresenham `draw_line`, full 8x8 font (ASCII 32–126), palette mapping for PC.
- **Games**: Snake (full), Pong, Tetris, Space Invaders, and Racing are implemented with drawing and input on PC; on PS2 they are stubs.

---

## Frameworks and enhancements

This is a **bare-metal OS** (no libc, no host OS). Traditional GUI frameworks (Qt, GTK, etc.) do not apply.

- **Internal structure**: Introduce a small **video abstraction** (e.g. `video.h` / `vga.c`) so that:
  - On PC/QEMU: use VGA mode 13h (or later VESA) framebuffer at 0xA0000.
  - On PS2: keep using PS2 VRAM and GS (current asm/C).
- **Libraries**: Only code that can be linked in a freestanding kernel is relevant: e.g. a small 8x8 font table, integer-only line/rect helpers, or a minimal tile/sprite helper. No heavy dependencies.
- **Tooling**: QEMU is the main "framework" for running and debugging the GUI on your machine; the build is already set up for that.

---

## Implemented

- **VGA mode 13h**: `include/video.h`, `src/vga.c` (port I/O, mode 13h and text mode 0x03, palette).
- **Framebuffer abstraction**: `video.h` + `graphics.c` use `FRAMEBUFFER_ADDR`, `VIDEO_*`; palette for PC.
- **Integer Bresenham** for `draw_line` in `graphics.c`.
- **Shell demo**: `cmd_demo` switches to mode 13h, runs demo, restores text mode.
- **Shell game**: `cmd_game` / `game <number>` call `list_games()` and `launch_game(index)`.
- **Full 8x8 font**: `src/font_8x8.c`, `include/font_8x8.h` (ASCII 32-126); `draw_char` uses it.
- **PC keyboard**: `src/keyboard.c`, `include/keyboard.h` (poll 0x60/0x64); used by demo and games when not PS2.
- **Snake game**: Full implementation in `game.c` (16x10 grid, arrow keys, food, score, game over).
- **Games in graphics mode**: `launch_game()` switches to mode 13h and sets palette (PC), then restores text after.
- **init_game_system at boot**: `kernel_main()` calls `init_game_system()` after `init_shell()`.

---

## What to do next (optional / future)

- **VESA path**: Add a VESA path for higher resolution (e.g. 640x480 32bpp) while keeping mode 13h as fallback for compatibility.
- **PS2 game/demo wiring**: When building with `PS2_HARDWARE`, wire controller input and GS/VRAM so the same games and demo run on real PS2 hardware.

---

## Summary

- **GUI** = the graphics demo and games (Snake, etc.) drawing in the framebuffer.  
- **Current state**: VGA mode 13h, palette, keyboard, font, demo and Snake implemented; games launch in graphics mode on PC.  
- **To run**: Install `nasm`, then `make all` and `make run` (QEMU). In shell: `demo` for bouncing-rectangles demo, `game` then `game 1` for Snake.
- **Tests**: From `asmos/` run `make test` for the full test suite (deps, C compilation, shell, FS, hardware, security, docs). Build/asm tests are skipped if `nasm` is missing. `make test-quick` runs a quick build test (requires nasm).
