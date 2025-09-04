# PS2 x86 OS Tools Installation Guide

## Overview
This guide will help you install all the missing tools needed for PS2 x86 OS development and testing.

## Required Tools

### Essential Tools (Required for Build)
- **make** - Build system
- **nasm** - Netwide Assembler
- **gcc** - GNU Compiler Collection
- **ld** - Linker (part of binutils)
- **objcopy** - Object file manipulation (part of binutils)

### Optional Tools (For Testing & Deployment)
- **qemu-system-i386** or **qemu-system-x86_64** - Emulation for testing
- **mkisofs** or **genisoimage** - ISO creation
- **growisofs** - CD/DVD burning
- **mkfs.fat** - FAT filesystem creation

## Quick Installation

### Option 1: Automated Installation Script

#### For Linux/WSL:
```bash
# Make the script executable
chmod +x install_tools.sh

# Run the installation script
bash install_tools.sh
```

#### For Windows:
```cmd
# Run the Windows installation script
install_tools_windows.bat
```

### Option 2: Manual Installation

#### Ubuntu/Debian:
```bash
sudo apt update
sudo apt install -y make nasm gcc binutils qemu-system-x86 genisoimage dvd+rw-tools dosfstools
```

#### CentOS/RHEL/Fedora:
```bash
sudo yum install -y make nasm gcc binutils qemu-system-x86 genisoimage dvd+rw-tools dosfstools
# OR for newer Fedora:
sudo dnf install -y make nasm gcc binutils qemu-system-x86 genisoimage dvd+rw-tools dosfstools
```

#### Arch Linux:
```bash
sudo pacman -S --noconfirm make nasm gcc binutils qemu cdrtools dosfstools
```

#### macOS:
```bash
# Install Homebrew first if you haven't
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install tools
brew install make nasm gcc binutils qemu cdrtools dosfstools
```

#### Windows (with Chocolatey):
```powershell
# Install Chocolatey first (run as Administrator)
Set-ExecutionPolicy Bypass -Scope Process -Force; [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))

# Install tools
choco install -y make nasm mingw git qemu cdrtools
```

## 🔧 Tool Compatibility Fixes

### QEMU Issues
If you get `qemu-system-i386: command not found`, try:
```bash
# Create symlink for qemu-system-i386
sudo ln -sf /usr/bin/qemu-system-x86_64 /usr/bin/qemu-system-i386
```

### ISO Creation Issues
If you get `mkisofs: command not found`, try:
```bash
# Create symlink for mkisofs
sudo ln -sf /usr/bin/genisoimage /usr/bin/mkisofs
```

## Testing Your Installation

After installation, run the test script:
```bash
# Make the test script executable
chmod +x test_build.sh

# Run the test
bash test_build.sh
```

This will verify that:
- All essential tools are available
- The build process works correctly
- QEMU can run the OS (if available)
- ISO creation works (if tools are available)

## Expected Test Results

### Successful Test Output:
```
=== PS2 x86 OS Build Test ===

Checking essential tools...
[✓] make is available
[✓] nasm is available
[✓] gcc is available
[✓] ld is available
[✓] objcopy is available

Checking optional tools...
[✓] qemu-system-i386 is available
[✓] mkisofs is available
[✓] growisofs is available
[✓] mkfs.fat is available

Testing build process...
[!] Running make clean...
[!] Running make all...
[✓] Build successful! Disk image created.
[✓] Size: 1474560 bytes

Testing QEMU...
[!] Starting QEMU (will run for 5 seconds)...
[✓] QEMU test completed

Testing ISO creation...
[!] Creating ISO...
[✓] ISO created successfully!
[✓] Size: 1474560 bytes

=== Test Complete! ===
Your PS2 x86 OS build system is working correctly! 
```

## Troubleshooting

### Common Issues:

#### 1. "make: command not found"
- **Solution**: Install build-essential package
- **Ubuntu/Debian**: `sudo apt install build-essential`

#### 2. "nasm: command not found"
- **Solution**: Install NASM assembler
- **Ubuntu/Debian**: `sudo apt install nasm`

#### 3. "qemu-system-i386: command not found"
- **Solution**: Install QEMU and create symlink
- **Ubuntu/Debian**: `sudo apt install qemu-system-x86 && sudo ln -sf /usr/bin/qemu-system-x86_64 /usr/bin/qemu-system-i386`

#### 4. "mkisofs: command not found"
- **Solution**: Install cdrtools and create symlink
- **Ubuntu/Debian**: `sudo apt install genisoimage && sudo ln -sf /usr/bin/genisoimage /usr/bin/mkisofs`

#### 5. "mkfs.fat: command not found"
- **Solution**: Install dosfstools
- **Ubuntu/Debian**: `sudo apt install dosfstools`

#### 6. Build Errors
- **Solution**: Check that all source files are present
- **Check**: Run `ls src/ boot/ include/` to verify files exist

#### 7. Permission Errors
- **Solution**: Make scripts executable
- **Command**: `chmod +x *.sh`

## Next Steps

After successful installation:

1. **Test the build**:
   ```bash
   make clean && make all
   ```

2. **Run in QEMU**:
   ```bash
   make run
   ```

3. **Create bootable ISO**:
   ```bash
   make iso
   ```

4. **Burn to CD** (Linux):
   ```bash
   growisofs -dvd-compat -Z /dev/sr0=disk/os.img
   ```

5. **Boot on PS2**:
   - Insert CD with burned ISO
   - Boot PS2 with modchip (DMS3)

## System Requirements

### Minimum Requirements:
- **OS**: Linux, macOS, or Windows (with WSL)
- **RAM**: 2GB minimum, 4GB recommended
- **Disk**: 1GB free space
- **CPU**: Any modern x86_64 processor

### Recommended Requirements:
- **OS**: Ubuntu 20.04+ or similar
- **RAM**: 8GB
- **Disk**: 5GB free space
- **CPU**: Multi-core processor

## PS2 Hardware Requirements

- **PlayStation 2 Console** (any model)
- **DMS3 Modchip** (or compatible modchip)
- **USB Keyboard** (for shell interaction)
- **CD/DVD Media** (CD-R recommended)
- **CD/DVD Burner**

## Support

If you encounter issues:

1. **Check the troubleshooting section above**
2. **Run the test script**: `bash test_build.sh`
3. **Check tool versions**: `make --version && nasm --version && gcc --version`
4. **Verify file permissions**: `ls -la *.sh`

## Success!

Once all tools are installed and tested, you'll have a complete PS2 x86 OS development environment ready for:
- Building the OS
- Testing in QEMU
- Creating bootable ISOs
- Burning to CD
- Booting on real PS2 hardware

