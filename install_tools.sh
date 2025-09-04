#!/bin/bash

# PS2 x86 OS Tools Installation Script
# Installs all missing tools for development and testing

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== PS2 x86 OS Tools Installation Script ===${NC}"
echo -e "${CYAN}Installing all missing tools for PS2 x86 OS development...${NC}"
echo ""

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to install package
install_package() {
    local package=$1
    local name=$2
    
    if command_exists "$package"; then
        echo -e "${GREEN}[✓]${NC} $name is already installed"
        return 0
    fi
    
    echo -e "${YELLOW}[!]${NC} Installing $name..."
    
    # Try different package managers
    if command_exists apt; then
        sudo apt update && sudo apt install -y "$package"
    elif command_exists yum; then
        sudo yum install -y "$package"
    elif command_exists dnf; then
        sudo dnf install -y "$package"
    elif command_exists pacman; then
        sudo pacman -S --noconfirm "$package"
    elif command_exists brew; then
        brew install "$package"
    else
        echo -e "${RED}[✗]${NC} No supported package manager found for $name"
        return 1
    fi
    
    if command_exists "$package"; then
        echo -e "${GREEN}[✓]${NC} $name installed successfully"
    else
        echo -e "${RED}[✗]${NC} Failed to install $name"
        return 1
    fi
}

# Function to install cross-compiler
install_cross_compiler() {
    echo -e "${YELLOW}[!]${NC} Installing cross-compiler tools..."
    
    if command_exists apt; then
        sudo apt update
        sudo apt install -y gcc-multilib gcc-i686-linux-gnu binutils-i686-linux-gnu
        echo -e "${GREEN}[✓]${NC} Cross-compiler tools installed"
    elif command_exists yum; then
        sudo yum install -y gcc-multilib gcc-i686-linux-gnu binutils-i686-linux-gnu
        echo -e "${GREEN}[✓]${NC} Cross-compiler tools installed"
    elif command_exists dnf; then
        sudo dnf install -y gcc-multilib gcc-i686-linux-gnu binutils-i686-linux-gnu
        echo -e "${GREEN}[✓]${NC} Cross-compiler tools installed"
    elif command_exists pacman; then
        sudo pacman -S --noconfirm gcc-multilib
        echo -e "${GREEN}[✓]${NC} Cross-compiler tools installed"
    elif command_exists brew; then
        brew install i686-elf-gcc i686-elf-binutils
        echo -e "${GREEN}[✓]${NC} Cross-compiler tools installed"
    else
        echo -e "${RED}[✗]${NC} No supported package manager found for cross-compiler"
    fi
}

# Check current system
echo -e "${BLUE}Detecting system...${NC}"
if command_exists apt; then
    echo -e "${GREEN}[✓]${NC} Ubuntu/Debian system detected"
    PKG_MANAGER="apt"
elif command_exists yum; then
    echo -e "${GREEN}[✓]${NC} CentOS/RHEL system detected"
    PKG_MANAGER="yum"
elif command_exists dnf; then
    echo -e "${GREEN}[✓]${NC} Fedora system detected"
    PKG_MANAGER="dnf"
elif command_exists pacman; then
    echo -e "${GREEN}[✓]${NC} Arch Linux system detected"
    PKG_MANAGER="pacman"
elif command_exists brew; then
    echo -e "${GREEN}[✓]${NC} macOS system detected"
    PKG_MANAGER="brew"
else
    echo -e "${RED}[✗]${NC} Unsupported system - please install tools manually"
    exit 1
fi

echo ""

# Install essential development tools
echo -e "${BLUE}Installing essential development tools...${NC}"
install_package "make" "Make"
install_package "nasm" "NASM Assembler"
install_package "gcc" "GCC Compiler"
install_package "ld" "Linker (binutils)"

# Install cross-compiler tools
install_cross_compiler

# Install QEMU for emulation
echo -e "${BLUE}Installing QEMU for emulation...${NC}"
if command_exists apt; then
    install_package "qemu-system-x86" "QEMU System Emulator"
elif command_exists yum; then
    install_package "qemu-system-x86" "QEMU System Emulator"
elif command_exists dnf; then
    install_package "qemu-system-x86" "QEMU System Emulator"
elif command_exists pacman; then
    install_package "qemu" "QEMU System Emulator"
elif command_exists brew; then
    install_package "qemu" "QEMU System Emulator"
fi

# Install ISO creation tools
echo -e "${BLUE}Installing ISO creation tools...${NC}"
if command_exists apt; then
    install_package "genisoimage" "ISO Creation Tool"
elif command_exists yum; then
    install_package "genisoimage" "ISO Creation Tool"
elif command_exists dnf; then
    install_package "genisoimage" "ISO Creation Tool"
elif command_exists pacman; then
    install_package "cdrtools" "ISO Creation Tool"
elif command_exists brew; then
    install_package "cdrtools" "ISO Creation Tool"
fi

# Install CD/DVD burning tools
echo -e "${BLUE}Installing CD/DVD burning tools...${NC}"
if command_exists apt; then
    install_package "growisofs" "DVD Burning Tool"
elif command_exists yum; then
    install_package "dvd+rw-tools" "DVD Burning Tool"
elif command_exists dnf; then
    install_package "dvd+rw-tools" "DVD Burning Tool"
elif command_exists pacman; then
    install_package "dvd+rw-tools" "DVD Burning Tool"
elif command_exists brew; then
    install_package "dvd+rw-tools" "DVD Burning Tool"
fi

# Install filesystem tools
echo -e "${BLUE}Installing filesystem tools...${NC}"
if command_exists apt; then
    install_package "dosfstools" "FAT Filesystem Tools"
elif command_exists yum; then
    install_package "dosfstools" "FAT Filesystem Tools"
elif command_exists dnf; then
    install_package "dosfstools" "FAT Filesystem Tools"
elif command_exists pacman; then
    install_package "dosfstools" "FAT Filesystem Tools"
elif command_exists brew; then
    install_package "dosfstools" "FAT Filesystem Tools"
fi

# Install additional useful tools
echo -e "${BLUE}Installing additional useful tools...${NC}"
install_package "hexdump" "Hex Dump Tool"
install_package "xxd" "Hex Dump Tool"
install_package "file" "File Type Detector"

# Create symlinks for common tool names
echo -e "${BLUE}Creating tool symlinks...${NC}"
if command_exists qemu-system-x86_64 && ! command_exists qemu-system-i386; then
    echo -e "${YELLOW}[!]${NC} Creating symlink for qemu-system-i386"
    sudo ln -sf /usr/bin/qemu-system-x86_64 /usr/bin/qemu-system-i386
fi

if command_exists genisoimage && ! command_exists mkisofs; then
    echo -e "${YELLOW}[!]${NC} Creating symlink for mkisofs"
    sudo ln -sf /usr/bin/genisoimage /usr/bin/mkisofs
fi

echo ""

# Verify installations
echo -e "${BLUE}Verifying installations...${NC}"
TOOLS=("make" "nasm" "gcc" "ld" "qemu-system-i386" "mkisofs" "growisofs" "mkfs.fat" "hexdump" "file")

for tool in "${TOOLS[@]}"; do
    if command_exists "$tool"; then
        echo -e "${GREEN}[✓]${NC} $tool is available"
    else
        echo -e "${RED}[✗]${NC} $tool is missing"
    fi
done

echo ""

# Test QEMU
echo -e "${BLUE}Testing QEMU...${NC}"
if command_exists qemu-system-i386; then
    echo -e "${GREEN}[✓]${NC} QEMU is working"
    qemu-system-i386 --version | head -1
else
    echo -e "${RED}[✗]${NC} QEMU is not working"
fi

echo ""

# Update Makefile if needed
echo -e "${BLUE}Updating Makefile for tool compatibility...${NC}"
if [ -f "Makefile" ]; then
    # Check if we need to update the Makefile for different tool names
    if command_exists genisoimage && ! command_exists mkisofs; then
        echo -e "${YELLOW}[!]${NC} Updating Makefile to use genisoimage instead of mkisofs"
        sed -i 's/mkisofs/genisoimage/g' Makefile
    fi
    
    if command_exists qemu-system-x86_64 && ! command_exists qemu-system-i386; then
        echo -e "${YELLOW}[!]${NC} Updating Makefile to use qemu-system-x86_64"
        sed -i 's/qemu-system-i386/qemu-system-x86_64/g' Makefile
    fi
fi

echo ""
echo -e "${GREEN}=== Installation Complete! ===${NC}"
echo -e "${CYAN}All tools have been installed for PS2 x86 OS development.${NC}"
echo ""
echo -e "${BLUE}Next steps:${NC}"
echo "1. Run: make clean && make all"
echo "2. Test with: make run"
echo "3. Create ISO: make iso"
echo "4. Burn to CD: growisofs -dvd-compat -Z /dev/sr0=disk/os.img"
echo ""
echo -e "${PURPLE}Happy PS2 x86 OS development! 🎮${NC}"
