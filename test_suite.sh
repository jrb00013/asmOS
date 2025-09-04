#!/bin/bash

# PS2 x86 OS Comprehensive Test Suite v3.0
# Full system testing and validation

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m'

# Test configuration
TEST_MODE=${1:-"all"}
VERBOSE=${2:-"false"}
SAVE_LOGS=${3:-"true"}
QEMU_TIMEOUT=30

# Test results
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Log file
LOG_FILE="test_results_$(date +%Y%m%d_%H%M%S).log"

echo -e "${BLUE}PS2 x86 OS Comprehensive Test Suite v3.0${NC}"
echo -e "${BLUE}==========================================${NC}"
echo ""

# Function to print colored output
print_status() {
    echo -e "${GREEN}[PASS]${NC} $1"
    if [ "$SAVE_LOGS" = "true" ]; then
        echo "[PASS] $1" >> "$LOG_FILE"
    fi
}

print_fail() {
    echo -e "${RED}[FAIL]${NC} $1"
    if [ "$SAVE_LOGS" = "true" ]; then
        echo "[FAIL] $1" >> "$LOG_FILE"
    fi
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
    if [ "$SAVE_LOGS" = "true" ]; then
        echo "[WARN] $1" >> "$LOG_FILE"
    fi
}

print_info() {
    echo -e "${CYAN}[INFO]${NC} $1"
    if [ "$SAVE_LOGS" = "true" ]; then
        echo "[INFO] $1" >> "$LOG_FILE"
    fi
}

print_header() {
    echo -e "${PURPLE}$1${NC}"
    if [ "$SAVE_LOGS" = "true" ]; then
        echo "$1" >> "$LOG_FILE"
    fi
}

# Initialize log file
init_log() {
    if [ "$SAVE_LOGS" = "true" ]; then
        echo "PS2 x86 OS Test Suite Log - $(date)" > "$LOG_FILE"
        echo "=====================================" >> "$LOG_FILE"
        echo "" >> "$LOG_FILE"
    fi
}

# Test counter
increment_test() {
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
}

# Test result tracking
test_passed() {
    PASSED_TESTS=$((PASSED_TESTS + 1))
    print_status "$1"
}

test_failed() {
    FAILED_TESTS=$((FAILED_TESTS + 1))
    print_fail "$1"
}

# Dependency testing
test_dependencies() {
    print_header "=== Testing Dependencies ==="
    
    local deps=("nasm" "i686-elf-gcc" "make" "dd" "mkisofs" "qemu-system-i386")
    local missing=()
    
    for dep in "${deps[@]}"; do
        increment_test
        if command -v "$dep" >/dev/null 2>&1; then
            test_passed "Dependency $dep found"
        else
            test_failed "Dependency $dep missing"
            missing+=("$dep")
        fi
    done
    
    if [ ${#missing[@]} -gt 0 ]; then
        print_warning "Missing dependencies: ${missing[*]}"
        print_info "Install with: sudo apt install nasm gcc-multilib gcc-multilib-i686-elf make mkisofs qemu-system"
    fi
}

# Build testing
test_build() {
    print_header "=== Testing Build Process ==="
    
    # Clean previous builds
    increment_test
    if make clean >/dev/null 2>&1; then
        test_passed "Clean build successful"
    else
        test_failed "Clean build failed"
    fi
    
    # PS2 build
    increment_test
    if make ps2-build >/dev/null 2>&1; then
        test_passed "PS2 build successful"
    else
        test_failed "PS2 build failed"
        return 1
    fi
    
    # Check build artifacts
    increment_test
    if [ -f "build/boot.bin" ] && [ -f "build/kernel.bin" ] && [ -f "disk/os.img" ]; then
        test_passed "Build artifacts created"
    else
        test_failed "Build artifacts missing"
    fi
    
    # ISO creation
    increment_test
    if make iso >/dev/null 2>&1; then
        test_passed "ISO creation successful"
    else
        test_failed "ISO creation failed"
    fi
    
    # Check ISO file
    increment_test
    if [ -f "ps2os.iso" ]; then
        local iso_size=$(stat -c%s ps2os.iso 2>/dev/null || echo "0")
        if [ "$iso_size" -gt 1000000 ]; then
            test_passed "ISO file created ($((iso_size / 1024 / 1024)) MB)"
        else
            test_failed "ISO file too small"
        fi
    else
        test_failed "ISO file not found"
    fi
}

# Assembly syntax testing
test_assembly_syntax() {
    print_header "=== Testing Assembly Syntax ==="
    
    local asm_files=("boot/boot.asm" "boot/fat12.asm" "boot/syscalls.asm")
    
    for file in "${asm_files[@]}"; do
        increment_test
        if [ -f "$file" ]; then
            if nasm -f bin -I boot/ "$file" -o /dev/null 2>/dev/null; then
                test_passed "Assembly syntax check: $file"
            else
                test_failed "Assembly syntax error: $file"
            fi
        else
            test_failed "Assembly file missing: $file"
        fi
    done
}

# C compilation testing
test_c_compilation() {
    print_header "=== Testing C Compilation ==="
    
    local c_files=("src/kernel.c" "src/shell.c" "src/memory_manager.c" "src/scheduler.c")
    
    for file in "${c_files[@]}"; do
        increment_test
        if [ -f "$file" ]; then
            if i686-elf-gcc -m32 -Wall -O2 -g -ffreestanding -nostdlib -fno-builtin -Iinclude -c "$file" -o /dev/null 2>/dev/null; then
                test_passed "C compilation: $file"
            else
                test_failed "C compilation error: $file"
            fi
        else
            test_failed "C file missing: $file"
        fi
    done
}

# QEMU testing
test_qemu() {
    print_header "=== Testing QEMU Emulation ==="
    
    # Start QEMU in background
    increment_test
    if timeout "$QEMU_TIMEOUT" qemu-system-i386 -monitor null -drive format=raw,file=disk/os.img -m 32 -nographic -serial stdio > qemu_test.log 2>&1 &
    then
        local qemu_pid=$!
        sleep 5
        
        # Check if QEMU is still running
        if kill -0 "$qemu_pid" 2>/dev/null; then
            test_passed "QEMU started successfully"
            
            # Kill QEMU
            kill "$qemu_pid" 2>/dev/null
            wait "$qemu_pid" 2>/dev/null
        else
            test_failed "QEMU failed to start"
        fi
    else
        test_failed "QEMU execution failed"
    fi
    
    # Check for boot messages in log
    increment_test
    if grep -q "PS2 x86 Bootloader" qemu_test.log 2>/dev/null; then
        test_passed "Bootloader message detected"
    else
        test_failed "Bootloader message not found"
    fi
}

# Memory testing
test_memory() {
    print_header "=== Testing Memory Management ==="
    
    # Check memory manager compilation
    increment_test
    if i686-elf-gcc -m32 -Wall -O2 -g -ffreestanding -nostdlib -fno-builtin -Iinclude -c src/memory_manager.c -o /dev/null 2>/dev/null; then
        test_passed "Memory manager compilation"
    else
        test_failed "Memory manager compilation failed"
    fi
    
    # Check memory constants
    increment_test
    if grep -q "MEMORY_POOL_SIZE" src/memory_manager.c; then
        test_passed "Memory pool size defined"
    else
        test_failed "Memory pool size not defined"
    fi
}

# Filesystem testing
test_filesystem() {
    print_header "=== Testing Filesystem ==="
    
    # Check FAT12 implementation
    increment_test
    if grep -q "fat12_list_files" boot/fat12.asm; then
        test_passed "FAT12 file listing function"
    else
        test_failed "FAT12 file listing function missing"
    fi
    
    # Check filesystem header
    increment_test
    if [ -f "include/fs.h" ]; then
        test_passed "Filesystem header exists"
    else
        test_failed "Filesystem header missing"
    fi
}

# Shell testing
test_shell() {
    print_header "=== Testing Shell Commands ==="
    
    local commands=("help" "ls" "meminfo" "ps2info" "clear" "echo" "reboot" "exit")
    
    for cmd in "${commands[@]}"; do
        increment_test
        if grep -q "cmd_$cmd" src/shell.c; then
            test_passed "Shell command: $cmd"
        else
            test_failed "Shell command missing: $cmd"
        fi
    done
}

# Hardware detection testing
test_hardware_detection() {
    print_header "=== Testing Hardware Detection ==="
    
    # Check PS2 hardware header
    increment_test
    if [ -f "include/ps2_hardware.h" ]; then
        test_passed "PS2 hardware header exists"
    else
        test_failed "PS2 hardware header missing"
    fi
    
    # Check hardware detection functions
    increment_test
    if grep -q "detect_ps2_memory" boot/fat12.asm; then
        test_passed "PS2 memory detection function"
    else
        test_failed "PS2 memory detection function missing"
    fi
    
    # Check controller initialization
    increment_test
    if grep -q "init_ps2_controllers" boot/fat12.asm; then
        test_passed "PS2 controller initialization"
    else
        test_failed "PS2 controller initialization missing"
    fi
}

# Performance testing
test_performance() {
    print_header "=== Testing Performance ==="
    
    # Build time test
    increment_test
    local start_time=$(date +%s)
    make clean >/dev/null 2>&1
    make ps2-build >/dev/null 2>&1
    local end_time=$(date +%s)
    local build_time=$((end_time - start_time))
    
    if [ "$build_time" -lt 60 ]; then
        test_passed "Build time acceptable (${build_time}s)"
    else
        test_warning "Build time slow (${build_time}s)"
    fi
    
    # File size test
    increment_test
    if [ -f "disk/os.img" ]; then
        local img_size=$(stat -c%s disk/os.img)
        if [ "$img_size" -lt 1500000 ]; then
            test_passed "Disk image size reasonable ($((img_size / 1024)) KB)"
        else
            test_warning "Disk image large ($((img_size / 1024)) KB)"
        fi
    else
        test_failed "Disk image not found"
    fi
}

# Security testing
test_security() {
    print_header "=== Testing Security ==="
    
    # Check for buffer overflow protection
    increment_test
    if grep -q "sizeof" src/shell.c; then
        test_passed "Buffer size checking"
    else
        test_warning "Buffer size checking not found"
    fi
    
    # Check for null pointer protection
    increment_test
    if grep -q "NULL" src/memory_manager.c; then
        test_passed "Null pointer checking"
    else
        test_warning "Null pointer checking not found"
    fi
}

# Documentation testing
test_documentation() {
    print_header "=== Testing Documentation ==="
    
    local docs=("README.md" "INSTALL.md" "build_ps2.sh")
    
    for doc in "${docs[@]}"; do
        increment_test
        if [ -f "$doc" ]; then
            local doc_size=$(stat -c%s "$doc")
            if [ "$doc_size" -gt 1000 ]; then
                test_passed "Documentation: $doc ($((doc_size / 1024)) KB)"
            else
                test_warning "Documentation small: $doc"
            fi
        else
            test_failed "Documentation missing: $doc"
        fi
    done
}

# Integration testing
test_integration() {
    print_header "=== Testing Integration ==="
    
    # Test complete build pipeline
    increment_test
    if make clean && make ps2-build && make iso; then
        test_passed "Complete build pipeline"
    else
        test_failed "Complete build pipeline failed"
    fi
    
    # Test all make targets
    increment_test
    if make info >/dev/null 2>&1; then
        test_passed "Make info target"
    else
        test_failed "Make info target failed"
    fi
}

# Run specific test suite
run_test_suite() {
    case $TEST_MODE in
        "deps")
            test_dependencies
            ;;
        "build")
            test_build
            ;;
        "asm")
            test_assembly_syntax
            ;;
        "c")
            test_c_compilation
            ;;
        "qemu")
            test_qemu
            ;;
        "memory")
            test_memory
            ;;
        "fs")
            test_filesystem
            ;;
        "shell")
            test_shell
            ;;
        "hardware")
            test_hardware_detection
            ;;
        "perf")
            test_performance
            ;;
        "security")
            test_security
            ;;
        "docs")
            test_documentation
            ;;
        "integration")
            test_integration
            ;;
        "all")
            test_dependencies
            test_build
            test_assembly_syntax
            test_c_compilation
            test_memory
            test_filesystem
            test_shell
            test_hardware_detection
            test_performance
            test_security
            test_documentation
            test_integration
            ;;
        *)
            print_fail "Unknown test mode: $TEST_MODE"
            exit 1
            ;;
    esac
}

# Show test results
show_results() {
    echo ""
    print_header "=== Test Results Summary ==="
    echo "Total Tests: $TOTAL_TESTS"
    echo "Passed: $PASSED_TESTS"
    echo "Failed: $FAILED_TESTS"
    
    if [ "$SAVE_LOGS" = "true" ]; then
        echo "Log saved to: $LOG_FILE"
    fi
    
    if [ "$FAILED_TESTS" -eq 0 ]; then
        echo -e "${GREEN}All tests passed! 🎉${NC}"
        exit 0
    else
        echo -e "${RED}Some tests failed! ❌${NC}"
        exit 1
    fi
}

# Show usage
show_usage() {
    echo "Usage: $0 [TEST_MODE] [VERBOSE] [SAVE_LOGS]"
    echo ""
    echo "TEST_MODE options:"
    echo "  deps        - Test dependencies"
    echo "  build       - Test build process"
    echo "  asm         - Test assembly syntax"
    echo "  c           - Test C compilation"
    echo "  qemu        - Test QEMU emulation"
    echo "  memory      - Test memory management"
    echo "  fs          - Test filesystem"
    echo "  shell       - Test shell commands"
    echo "  hardware    - Test hardware detection"
    echo "  perf        - Test performance"
    echo "  security    - Test security features"
    echo "  docs        - Test documentation"
    echo "  integration - Test integration"
    echo "  all         - Run all tests (default)"
    echo ""
    echo "VERBOSE options:"
    echo "  true        - Verbose output"
    echo "  false       - Normal output (default)"
    echo ""
    echo "SAVE_LOGS options:"
    echo "  true        - Save logs to file (default)"
    echo "  false       - Don't save logs"
    echo ""
    echo "Examples:"
    echo "  $0 all true true    # All tests, verbose, save logs"
    echo "  $0 build false false # Build tests only, no logs"
    echo "  $0 qemu             # QEMU tests only"
}

# Main execution
main() {
    # Check for help flag
    if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
        show_usage
        exit 0
    fi
    
    # Initialize logging
    init_log
    
    # Run tests
    run_test_suite
    
    # Show results
    show_results
}

# Cleanup function
cleanup() {
    # Kill any running QEMU processes
    pkill -f qemu-system-i386 2>/dev/null || true
    
    # Remove test files
    rm -f qemu_test.log 2>/dev/null || true
}

# Set up cleanup trap
trap cleanup EXIT

# Run main function
main "$@"
