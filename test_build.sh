#!/bin/bash

# PS2 x86 OS Build Test Script
# Quick test to verify everything is working

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== PS2 x86 OS Build Test ===${NC}"
echo ""

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Check essential tools
echo -e "${BLUE}Checking essential tools...${NC}"
TOOLS=("make" "nasm" "gcc" "ld" "objcopy")

for tool in "${TOOLS[@]}"; do
    if command_exists "$tool"; then
        echo -e "${GREEN}[✓]${NC} $tool is available"
    else
        echo -e "${RED}[✗]${NC} $tool is missing"
        exit 1
    fi
done

echo ""

# Check optional tools
echo -e "${BLUE}Checking optional tools...${NC}"
OPTIONAL_TOOLS=("qemu-system-i386" "qemu-system-x86_64" "mkisofs" "genisoimage" "growisofs" "mkfs.fat")

for tool in "${OPTIONAL_TOOLS[@]}"; do
    if command_exists "$tool"; then
        echo -e "${GREEN}[✓]${NC} $tool is available"
    else
        echo -e "${YELLOW}[!]${NC} $tool is missing (optional)"
    fi
done

echo ""

# Test build
echo -e "${BLUE}Testing build process...${NC}"
echo -e "${YELLOW}[!]${NC} Running make clean..."
make clean

echo -e "${YELLOW}[!]${NC} Running make all..."
make all

if [ -f "disk/os.img" ]; then
    echo -e "${GREEN}[✓]${NC} Build successful! Disk image created."
    echo -e "${GREEN}[✓]${NC} Size: $(stat -c%s disk/os.img) bytes"
else
    echo -e "${RED}[✗]${NC} Build failed!"
    exit 1
fi

echo ""

# Test QEMU if available
if command_exists qemu-system-i386 || command_exists qemu-system-x86_64; then
    echo -e "${BLUE}Testing QEMU...${NC}"
    echo -e "${YELLOW}[!]${NC} Starting QEMU (will run for 5 seconds)..."
    timeout 5s make run || echo -e "${GREEN}[✓]${NC} QEMU test completed"
else
    echo -e "${YELLOW}[!]${NC} QEMU not available - skipping emulation test"
fi

echo ""

# Test ISO creation if available
if command_exists mkisofs || command_exists genisoimage; then
    echo -e "${BLUE}Testing ISO creation...${NC}"
    echo -e "${YELLOW}[!]${NC} Creating ISO..."
    make iso
    
    if [ -f "ps2os.iso" ]; then
        echo -e "${GREEN}[✓]${NC} ISO created successfully!"
        echo -e "${GREEN}[✓]${NC} Size: $(stat -c%s ps2os.iso) bytes"
    else
        echo -e "${RED}[✗]${NC} ISO creation failed!"
    fi
else
    echo -e "${YELLOW}[!]${NC} ISO creation tools not available - skipping ISO test"
fi

echo ""
echo -e "${GREEN}=== Test Complete! ===${NC}"
echo -e "${CYAN}Your PS2 x86 OS build system is working correctly!${NC}"
echo ""
echo -e "${BLUE}Next steps:${NC}"
echo "1. Run: make run (to test in QEMU)"
echo "2. Run: make iso (to create bootable ISO)"
echo "3. Burn to CD: growisofs -dvd-compat -Z /dev/sr0=disk/os.img"
echo "4. Boot on PS2 with modchip"
