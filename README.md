# PS2 x86 Assembly Real-Mode OS v2.0 - Enhanced Edition

A fully-featured x86 Assembly Real-Mode Operating System specifically optimized for PlayStation 2 (PS2) hardware with modchip support.

## PS2 Compatibility

This OS is designed to run on PlayStation 2 consoles with the following specifications:
- **CPU**: MIPS R5900 (Emotion Engine) - x86 emulation via modchip
- **Memory**: 32MB DDR SDRAM
- **Storage**: CD/DVD Drive
- **Controllers**: DualShock 2 Support
- **Network**: Ethernet (optional)

## Requirements

### Hardware Requirements
- **PS2 Console** (any model)
- **DMS3 Modchip** (or any compatible modchip)
- **CD/DVD Media** for burning the OS
- **USB Keyboard** (for shell interaction)

### Software Requirements
- **Linux Terminal** (Ubuntu/Debian recommended)
- **NASM** (Netwide Assembler) for Assembly compilation
- **GCC** (GNU Compiler Collection) for C compilation
- **i686-elf-gcc** cross-compiler
- **i686-elf-binutils** for linking
- **mkisofs** for ISO creation
- **growisofs** for CD burning

## Quick Start

### 1. Install Dependencies
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install nasm gcc make dd mkisofs growisofs

# Install cross-compiler
sudo apt-get install gcc-multilib gcc-multilib-i686-elf
```

### 2. Build the OS
```bash
# Clone the repository
git clone <repository-url>
cd asmOS

# Build for PS2 (optimized)
make ps2-build

# Or build with debugging
make all
```

### 3. Create Bootable CD
```bash
# Create ISO image
make iso

# Burn to CD (replace /dev/sr0 with your CD drive)
growisofs -dvd-compat -Z /dev/sr0=ps2os.iso
```

### 4. Boot on PS2
1. Insert the burned CD into your PS2
2. Power on the PS2 with modchip enabled
3. The OS will boot automatically
4. Use keyboard to interact with the shell

## Build Options

### Available Make Targets
- `make all` - Standard build
- `make ps2-build` - PS2-optimized build
- `make ps2-test` - Test PS2 build in QEMU
- `make iso` - Create bootable ISO
- `make clean` - Clean build artifacts
- `make info` - Show build information

### PS2-Specific Optimizations
- **CPU**: Optimized for Pentium III era (PS2 x86 emulation)
- **Memory**: 32MB RAM allocation
- **Graphics**: VGA text mode compatibility
- **Storage**: FAT12 filesystem for CD compatibility

## Features

### Enhanced Bootloader
- PS2 hardware detection
- Enhanced error handling
- Larger kernel support (16KB)
- Improved disk I/O

### Advanced Kernel
- PS2-specific optimizations
- Enhanced memory management
- Improved task scheduling
- Better system information display

### Rich Shell Environment
- **ps2info** - PS2 hardware information
- **meminfo** - Memory usage details
- **ls** - File listing with sizes
- **clear** - Screen clearing
- **reboot** - System restart
- **help** - Command help system

### Filesystem Support
- FAT12 filesystem for CD compatibility
- File listing with detailed information
- Directory navigation (planned)
- File reading capabilities (planned)

## System Commands

### Basic Commands
```bash
help          # Show available commands
ls            # List files
clear         # Clear screen
echo <text>   # Echo text
exit          # Exit shell
```

### System Information
```bash
ps2info       # PS2 hardware information
meminfo       # Memory usage information
date          # Current date/time
```

### System Control
```bash
reboot        # Reboot system
```

## Debugging

### QEMU Testing
```bash
# Test in QEMU before burning to CD
make ps2-test

# Debug with GDB
make debug
# In another terminal: make gdb
```

### Common Issues
1. **CD not booting**: Ensure modchip is properly installed
2. **Build errors**: Check cross-compiler installation
3. **Memory issues**: Verify 32MB RAM allocation
4. **Keyboard not working**: Check USB keyboard compatibility

## Advanced Configuration

### Customizing for Different PS2 Models
Edit `boot/boot.asm` to adjust:
- Memory detection thresholds
- Hardware compatibility flags
- Boot messages

### Adding New Commands
Edit `src/shell.c` to add:
- New command functions
- Command descriptions
- Help text

### Optimizing Performance
Edit `Makefile` to adjust:
- Compiler optimization flags
- CPU architecture settings
- Memory allocation

## Technical Details

### Boot Process
1. **BIOS Load**: PS2 BIOS loads bootloader from CD
2. **Hardware Detection**: Detect PS2-specific hardware
3. **Kernel Load**: Load kernel from FAT12 filesystem
4. **System Init**: Initialize memory, scheduler, shell
5. **User Interface**: Present enhanced shell to user

### Memory Layout
- **0x0000-0x7BFF**: Bootloader
- **0x7C00-0x7DFF**: Boot sector
- **0x10000-0x1FFFF**: Kernel (16KB)
- **0x20000+**: User space and system data

### Filesystem
- **FAT12**: Compatible with CD media
- **Boot Sector**: Enhanced with PS2 branding
- **Root Directory**: 224 entries maximum
- **File Size**: Up to 32MB per file


## License

This project is licensed under the MIT License - see the LICENSE file for details.


