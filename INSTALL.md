# PS2 x86 OS Installation Guide v2.0

## Prerequisites

### Hardware Requirements
- **PlayStation 2 Console** (any model: SCPH-10000, SCPH-15000, SCPH-18000, SCPH-30000, SCPH-35000, SCPH-37000, SCPH-39000, SCPH-50000, SCPH-70000, SCPH-75000, SCPH-77000, SCPH-79000, SCPH-90000)
- **DMS3 Modchip** (or compatible modchip: Matrix Infinity, Magic 3, Messiah, etc.)
- **USB Keyboard** (for shell interaction)
- **CD/DVD Media** (CD-R recommended for better compatibility)
- **CD/DVD Burner** (internal or external)

### Software Requirements
- **Linux Distribution** (Ubuntu 20.04+ or Debian 11+ recommended)
- **Internet Connection** (for downloading dependencies)

## 🔧 Installation Steps

### Step 1: Prepare Your Linux Environment

#### Install Required Packages
```bash
# Update package list
sudo apt update

# Install essential build tools
sudo apt install build-essential nasm gcc make

# Install cross-compiler for x86
sudo apt install gcc-multilib gcc-multilib-i686-elf

# Install disk utilities
sudo apt install dd mkisofs growisofs

# Install additional tools
sudo apt install git wget curl
```

#### Verify Installation
```bash
# Check NASM version
nasm --version

# Check GCC version
gcc --version

# Check cross-compiler
i686-elf-gcc --version

# Check make
make --version
```

### Step 2: Download and Build PS2 x86 OS

#### Clone the Repository
```bash
# Clone the repository
git clone <repository-url>
cd asmOS

# Make build script executable
chmod +x build_ps2.sh
```

#### Build the OS
```bash
# Option 1: Use the build script (recommended)
./build_ps2.sh ps2 true true

# Option 2: Use make directly
make ps2-build
make iso
```

#### Verify Build Success
```bash
# Check if files were created
ls -la disk/
ls -la ps2os.iso

# Check file sizes
du -h disk/os.img
du -h ps2os.iso
```

### Step 3: Burn to CD/DVD

#### Identify Your CD/DVD Drive
```bash
# List available drives
lsblk

# Or check for CD drives
ls /dev/sr*

# Common drive names:
# /dev/sr0 - First CD/DVD drive
# /dev/sr1 - Second CD/DVD drive
# /dev/cdrom - Symbolic link to CD drive
```

#### Burn the ISO
```bash
# Replace /dev/sr0 with your actual drive
sudo growisofs -dvd-compat -Z /dev/sr0=ps2os.iso

# Alternative burning methods:
# Using wodim (if growisofs fails)
sudo wodim -v -eject ps2os.iso

# Using brasero (GUI)
brasero ps2os.iso
```

#### Verify Burn Success
```bash
# Check if CD was burned correctly
sudo dd if=/dev/sr0 of=verify.iso bs=4M count=1
diff ps2os.iso verify.iso
```

### Step 4: Install Modchip (if not already installed)

#### DMS3 Modchip Installation
**WARNING: This requires opening your PS2 console. Proceed with caution.**

1. **Disconnect PS2** from power and TV
2. **Open PS2 case** (requires special screwdriver)
3. **Locate modchip installation points** (varies by PS2 model)
4. **Install modchip** according to manufacturer instructions
5. **Close PS2 case** and reconnect

#### Alternative: Use Existing Modchip
If you already have a modchip installed:
1. **Verify modchip is working** with a test disc
2. **Ensure modchip supports** x86 emulation
3. **Check modchip firmware** is up to date

### Step 5: Boot PS2 x86 OS

#### Prepare PS2 Console
1. **Connect USB keyboard** to PS2
2. **Insert burned CD** into PS2
3. **Connect PS2 to TV** (VGA or component recommended)
4. **Power on PS2** with modchip enabled

#### Boot Process
1. **PS2 BIOS loads** from CD
2. **Modchip intercepts** boot process
3. **PS2 x86 OS bootloader** starts
4. **Hardware detection** runs
5. **Kernel loads** and initializes
6. **Shell appears** ready for commands

#### Expected Boot Sequence
```
PS2 x86 Bootloader v4.0 - Enhanced Edition
PS2 hardware detected. Optimizing for PS2...
Kernel loaded successfully! Transferring control...
PS2 x86 OS Kernel v2.0 - Enhanced Edition
Detecting PS2 hardware...
PS2 detected! Total memory: 32 MB
Initializing memory manager...
Loading kernel from disk...
Initializing scheduler and FAT12 filesystem...
Initializing enhanced shell...
PS2 x86 OS Kernel v2.0 - Ready!
PS2OS [user@ps2]:~$ 
```

## Troubleshooting

### Build Issues

#### Missing Dependencies
```bash
# Error: nasm: command not found
sudo apt install nasm

# Error: i686-elf-gcc: command not found
sudo apt install gcc-multilib gcc-multilib-i686-elf

# Error: make: command not found
sudo apt install build-essential
```

#### Compilation Errors
```bash
# Clean and rebuild
make clean
make ps2-build

# Check for specific errors
make all 2>&1 | grep -i error
```

### Burning Issues

#### Drive Not Found
```bash
# Check if drive is recognized
sudo dmesg | grep -i cd

# Try different drive names
sudo growisofs -dvd-compat -Z /dev/cdrom=ps2os.iso
sudo growisofs -dvd-compat -Z /dev/dvd=ps2os.iso
```

#### Burn Failures
```bash
# Use different media (CD-R instead of CD-RW)
# Use different burning speed
sudo growisofs -dvd-compat -speed=4 -Z /dev/sr0=ps2os.iso

# Try alternative burning software
sudo apt install wodim
sudo wodim -v -eject ps2os.iso
```

### Boot Issues

#### CD Not Booting
1. **Check modchip installation**
2. **Verify CD was burned correctly**
3. **Try different CD media**
4. **Check PS2 model compatibility**

#### Black Screen
1. **Check TV connection**
2. **Try different video cables**
3. **Check if PS2 is receiving power**
4. **Verify modchip is working**

#### Keyboard Not Working
1. **Try different USB keyboard**
2. **Check USB port compatibility**
3. **Restart PS2 with keyboard connected**
4. **Use PS2 keyboard adapter if available**

### Performance Issues

#### Slow Boot
1. **Check CD media quality**
2. **Clean CD drive lens**
3. **Use faster CD media**
4. **Verify modchip performance**

#### Memory Issues
1. **Check PS2 memory card**
2. **Remove unnecessary peripherals**
3. **Restart PS2**
4. **Check for memory card corruption**

## Testing and Verification

### QEMU Testing
```bash
# Test OS in emulator before burning
make ps2-test

# Debug with GDB
make debug
# In another terminal: make gdb
```

### Hardware Testing
```bash
# Test basic commands
help
ps2info
meminfo
ls

# Test system functions
clear
echo "Hello PS2!"
date
```

### Performance Testing
```bash
# Check memory usage
meminfo

# Test file operations
ls

# Test system stability
# Run for extended period
```

## Advanced Configuration

### Customizing Boot Messages
Edit `boot/boot.asm`:
```assembly
boot_msg db "Your Custom Boot Message", 0x0D, 0x0A, 0
```

### Adding New Commands
Edit `src/shell.c`:
```c
static void cmd_yourcommand(char *args) {
    kprintf("Your custom command: %s\n", args);
}
```

### Optimizing for Your PS2 Model
Edit `Makefile`:
```makefile
CFLAGS += -DPS2_MODEL_SCPH_50000
```

## Support

### Getting Help
1. **Check README.md** for basic information
2. **Review troubleshooting section** above
3. **Check build logs** for specific errors
4. **Test in QEMU** before burning to CD

### Common Questions

**Q: Will this work on my PS2 model?**
A: Yes, all PS2 models are supported with proper modchip installation.

**Q: Do I need a specific modchip?**
A: Any modchip that supports x86 emulation will work. DMS3 is recommended.

**Q: Can I use a USB drive instead of CD?**
A: No, this OS is designed for CD booting. USB booting is not supported.

**Q: Is this legal?**
A: Yes, this is a legitimate operating system. Modchip usage may vary by region.

## License and Legal

This software is provided as-is for educational and personal use. Users are responsible for compliance with local laws regarding modchip usage and console modification.

---

**Enjoy your enhanced PS2 x86 OS experience!**
