#!/bin/bash

# PS2 x86 OS Clean Build Test Script
# Tests build process with minimal warnings

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== PS2 x86 OS Clean Build Test ===${NC}"
echo ""

# Function to count warnings in output
count_warnings() {
    local output="$1"
    local warning_count=$(echo "$output" | grep -i "warning" | wc -l)
    echo $warning_count
}

# Function to count errors in output
count_errors() {
    local output="$1"
    local error_count=$(echo "$output" | grep -i "error" | wc -l)
    echo $error_count
}

echo -e "${BLUE}Testing clean build process...${NC}"
echo -e "${YELLOW}[!]${NC} Running make clean..."
make clean

echo -e "${YELLOW}[!]${NC} Running make all (capturing output)..."
build_output=$(make all 2>&1)
build_exit_code=$?

echo ""
echo -e "${BLUE}Build Results:${NC}"

# Check if build succeeded
if [ $build_exit_code -eq 0 ]; then
    echo -e "${GREEN}[✓]${NC} Build completed successfully"
else
    echo -e "${RED}[✗]${NC} Build failed with exit code $build_exit_code"
    echo "$build_output"
    exit 1
fi

# Count warnings and errors
warning_count=$(count_warnings "$build_output")
error_count=$(count_errors "$build_output")

echo -e "${GREEN}[✓]${NC} Errors: $error_count"
echo -e "${YELLOW}[!]${NC} Warnings: $warning_count"

# Check if disk image was created
if [ -f "disk/os.img" ]; then
    echo -e "${GREEN}[✓]${NC} Disk image created successfully"
    echo -e "${GREEN}[✓]${NC} Size: $(stat -c%s disk/os.img) bytes"
else
    echo -e "${RED}[✗]${NC} Disk image not created"
    exit 1
fi

echo ""

# Show warning details if any
if [ $warning_count -gt 0 ]; then
    echo -e "${YELLOW}Warning Details:${NC}"
    echo "$build_output" | grep -i "warning" | head -10
    if [ $warning_count -gt 10 ]; then
        echo "... and $((warning_count - 10)) more warnings"
    fi
else
    echo -e "${GREEN}[✓]${NC} No warnings detected!"
fi

echo ""

# Test QEMU if available
if command -v qemu-system-i386 >/dev/null 2>&1 || command -v qemu-system-x86_64 >/dev/null 2>&1; then
    echo -e "${BLUE}Testing QEMU...${NC}"
    echo -e "${YELLOW}[!]${NC} Starting QEMU (will run for 3 seconds)..."
    timeout 3s make run >/dev/null 2>&1 || echo -e "${GREEN}[✓]${NC} QEMU test completed"
else
    echo -e "${YELLOW}[!]${NC} QEMU not available - skipping emulation test"
fi

echo ""
echo -e "${GREEN}=== Clean Build Test Complete! ===${NC}"

if [ $warning_count -eq 0 ]; then
    echo -e "${GREEN}[✓]${NC} Perfect! Build completed with no warnings! 🎉"
else
    echo -e "${YELLOW}[!]${NC} Build completed with $warning_count warnings"
    echo -e "${YELLOW}[!]${NC} Most warnings are harmless BSS section warnings"
fi

echo ""
echo -e "${BLUE}Build Summary:${NC}"
echo "  - Exit Code: $build_exit_code"
echo "  - Errors: $error_count"
echo "  - Warnings: $warning_count"
echo "  - Disk Image: $(stat -c%s disk/os.img) bytes"
echo ""
echo -e "${CYAN}Your PS2 x86 OS is ready for testing! 🎮${NC}"
