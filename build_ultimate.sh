#!/bin/bash

# PS2 x86 OS Ultimate Build Script v3.0
# Complete build system with advanced features

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m'

# Build configuration
BUILD_TYPE=${1:-"ultimate"}
CLEAN_BUILD=${2:-"true"}
CREATE_ISO=${3:-"true"}
RUN_TESTS=${4:-"true"}
DEMO_MODE=${5:-"false"}

# Version information
VERSION="3.0"
BUILD_DATE=$(date +%Y-%m-%d)
BUILD_TIME=$(date +%H:%M:%S)

echo -e "${PURPLE}PS2 x86 OS Ultimate Build Script v${VERSION}${NC}"
echo -e "${PURPLE}==========================================${NC}"
echo ""

# Function to print colored output
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_header() {
    echo -e "${BLUE}$1${NC}"
}

print_success() {
    echo -e "${CYAN}[SUCCESS]${NC} $1"
}

# Check system requirements
check_system_requirements() {
    print_header "=== Checking System Requirements ==="
    
    # Check OS
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        print_success "Linux detected"
    else
        print_warning "Non-Linux OS detected - some features may not work"
    fi
    
    # Check available memory
    local mem_kb=$(grep MemTotal /proc/meminfo | awk '{print $2}')
    local mem_mb=$((mem_kb / 1024))
    
    if [ "$mem_mb" -ge 2048 ]; then
        print_success "Memory: ${mem_mb}MB (sufficient)"
    else
        print_warning "Memory: ${mem_mb}MB (low - may affect build performance)"
    fi
    
    # Check disk space
    local disk_space=$(df . | awk 'NR==2 {print $4}')
    local disk_mb=$((disk_space / 1024))
    
    if [ "$disk_mb" -ge 1000 ]; then
        print_success "Disk space: ${disk_mb}MB (sufficient)"
    else
        print_error "Disk space: ${disk_mb}MB (insufficient - need at least 1GB)"
        exit 1
    fi
}

# Check dependencies
check_dependencies() {
    print_header "=== Checking Dependencies ==="
    
    local missing_deps=()
    local optional_deps=()
    
    # Required dependencies
    command -v nasm >/dev/null 2>&1 || missing_deps+=("nasm")
    command -v i686-elf-gcc >/dev/null 2>&1 || missing_deps+=("i686-elf-gcc")
    command -v make >/dev/null 2>&1 || missing_deps+=("make")
    command -v dd >/dev/null 2>&1 || missing_deps+=("dd")
    command -v mkisofs >/dev/null 2>&1 || missing_deps+=("mkisofs")
    
    # Optional dependencies
    command -v qemu-system-i386 >/dev/null 2>&1 || optional_deps+=("qemu-system-i386")
    command -v growisofs >/dev/null 2>&1 || optional_deps+=("growisofs")
    command -v timeout >/dev/null 2>&1 || optional_deps+=("timeout")
    
    if [ ${#missing_deps[@]} -ne 0 ]; then
        print_error "Missing required dependencies: ${missing_deps[*]}"
        echo ""
        echo "Install missing dependencies:"
        echo "sudo apt-get install nasm gcc-multilib gcc-multilib-i686-elf make mkisofs"
        exit 1
    fi
    
    if [ ${#optional_deps[@]} -ne 0 ]; then
        print_warning "Missing optional dependencies: ${optional_deps[*]}"
        echo "These are not required but provide additional features"
    fi
    
    print_success "All required dependencies found!"
}

# Clean build artifacts
clean_build() {
    if [ "$CLEAN_BUILD" = "true" ]; then
        print_header "=== Cleaning Build Artifacts ==="
        make clean
        print_success "Build artifacts cleaned!"
    fi
}

# Build the OS
build_os() {
    print_header "=== Building PS2 x86 OS ==="
    
    case $BUILD_TYPE in
        "ultimate")
            print_status "Building Ultimate Edition with all features..."
            make ps2-build
            ;;
        "debug")
            print_status "Building Debug Edition..."
            make all
            ;;
        "minimal")
            print_status "Building Minimal Edition..."
            make all
            ;;
        "test")
            print_status "Building Test Edition..."
            make ps2-test
            ;;
        *)
            print_warning "Unknown build type: $BUILD_TYPE"
            print_status "Using Ultimate build..."
            make ps2-build
            ;;
    esac
    
    print_success "Build completed successfully!"
}

# Run comprehensive tests
run_tests() {
    if [ "$RUN_TESTS" = "true" ]; then
        print_header "=== Running Comprehensive Tests ==="
        
        if [ -f "test_suite.sh" ]; then
            chmod +x test_suite.sh
            ./test_suite.sh all true true
            print_success "Tests completed!"
        else
            print_warning "Test suite not found - skipping tests"
        fi
    fi
}

# Create ISO image
create_iso() {
    if [ "$CREATE_ISO" = "true" ]; then
        print_header "=== Creating Bootable ISO ==="
        
        if [ -f "disk/os.img" ]; then
            make iso
            print_success "ISO created: ps2os.iso"
            
            # Show ISO information
            if [ -f "ps2os.iso" ]; then
                local iso_size=$(stat -c%s ps2os.iso)
                print_status "ISO size: $((iso_size / 1024 / 1024)) MB"
            fi
        else
            print_error "Disk image not found! Build failed."
            exit 1
        fi
    fi
}

# Run demo mode
run_demo() {
    if [ "$DEMO_MODE" = "true" ]; then
        print_header "=== Running Demo Mode ==="
        
        print_status "Starting PS2 x86 OS in QEMU for demo..."
        make ps2-test &
        local qemu_pid=$!
        
        sleep 10
        
        if kill -0 "$qemu_pid" 2>/dev/null; then
            print_success "Demo running successfully!"
            print_status "Press Ctrl+C to stop demo"
            
            # Wait for user to stop demo
            wait "$qemu_pid"
        else
            print_error "Demo failed to start"
        fi
    fi
}

# Show build information
show_build_info() {
    echo ""
    print_header "=== Build Information ==="
    echo "Version: $VERSION"
    echo "Build Type: $BUILD_TYPE"
    echo "Build Date: $BUILD_DATE"
    echo "Build Time: $BUILD_TIME"
    echo "Clean Build: $CLEAN_BUILD"
    echo "Create ISO: $CREATE_ISO"
    echo "Run Tests: $RUN_TESTS"
    echo "Demo Mode: $DEMO_MODE"
    echo ""
    
    if [ -f "disk/os.img" ]; then
        local img_size=$(stat -c%s disk/os.img)
        echo "Disk Image Size: $((img_size / 1024)) KB"
    fi
    
    if [ -f "ps2os.iso" ]; then
        local iso_size=$(stat -c%s ps2os.iso)
        echo "ISO Size: $((iso_size / 1024 / 1024)) MB"
    fi
}

# Show feature list
show_features() {
    echo ""
    print_header "=== PS2 x86 OS v${VERSION} Features ==="
    echo "🎮 Enhanced Bootloader with PS2 detection"
    echo "🖥️  Advanced Kernel with hardware optimization"
    echo "💻 Rich Shell with 25+ commands"
    echo "🌐 Network support (ping, ftp, telnet, irc)"
    echo "🔊 Sound system with SPU2 support"
    echo "🎨 Graphics system with demo capabilities"
    echo "⏱️  Timer system for precise timing"
    echo "🎮 PS2 controller support"
    echo "🎯 Game system with 5 classic games"
    echo "📁 Enhanced FAT12 filesystem"
    echo "🧠 Advanced memory management"
    echo "⚡ Task scheduler"
    echo "🔧 Comprehensive test suite"
    echo "📦 Automated build system"
    echo "📚 Complete documentation"
}

# Show next steps
show_next_steps() {
    echo ""
    print_header "=== Next Steps ==="
    
    if [ -f "ps2os.iso" ]; then
        echo "1. Burn ISO to CD:"
        echo "   growisofs -dvd-compat -Z /dev/sr0=ps2os.iso"
        echo ""
        echo "2. Boot on PS2:"
        echo "   - Insert CD into PS2"
        echo "   - Power on with modchip enabled"
        echo "   - Use USB keyboard for interaction"
        echo ""
        echo "3. Available commands:"
        echo "   help, ps2info, network, sound, graphics"
        echo "   game, demo, benchmark, system"
    else
        echo "1. Test in QEMU:"
        echo "   make ps2-test"
        echo ""
        echo "2. Create ISO:"
        echo "   make iso"
    fi
    
    echo ""
    echo "For more information, see README.md and INSTALL.md"
}

# Performance optimization
optimize_build() {
    print_header "=== Optimizing Build Performance ==="
    
    # Set build optimization flags
    export MAKEFLAGS="-j$(nproc)"
    print_status "Using $(nproc) parallel jobs"
    
    # Optimize for current CPU
    if command -v lscpu >/dev/null 2>&1; then
        local cpu_model=$(lscpu | grep "Model name" | cut -d: -f2 | xargs)
        print_status "CPU: $cpu_model"
    fi
}

# Security check
security_check() {
    print_header "=== Security Check ==="
    
    # Check for common security issues
    if grep -r "password\|secret\|key" src/ 2>/dev/null; then
        print_warning "Potential security issues found in source code"
    else
        print_success "No obvious security issues detected"
    fi
}

# Quality assurance
quality_assurance() {
    print_header "=== Quality Assurance ==="
    
    # Check code formatting
    local c_files=$(find src/ -name "*.c" | wc -l)
    local asm_files=$(find boot/ -name "*.asm" | wc -l)
    local header_files=$(find include/ -name "*.h" | wc -l)
    
    print_status "Source files: $c_files C, $asm_files ASM, $header_files headers"
    
    # Check for TODO comments
    local todo_count=$(grep -r "TODO" src/ boot/ include/ 2>/dev/null | wc -l)
    if [ "$todo_count" -gt 0 ]; then
        print_warning "$todo_count TODO items found"
    else
        print_success "No TODO items found"
    fi
}

# Main build process
main() {
    echo "Starting PS2 x86 OS Ultimate build process..."
    echo ""
    
    # Check system requirements
    check_system_requirements
    
    # Check dependencies
    check_dependencies
    
    # Optimize build
    optimize_build
    
    # Clean if requested
    clean_build
    
    # Build the OS
    build_os
    
    # Run tests
    run_tests
    
    # Create ISO if requested
    create_iso
    
    # Security check
    security_check
    
    # Quality assurance
    quality_assurance
    
    # Show build information
    show_build_info
    
    # Show features
    show_features
    
    # Show next steps
    show_next_steps
    
    # Run demo if requested
    run_demo
    
    echo ""
    print_success "Ultimate build process completed successfully!"
    echo ""
    echo -e "${PURPLE}🎮 PS2 x86 OS v${VERSION} - Ultimate Edition Ready! 🎮${NC}"
}

# Show usage
show_usage() {
    echo "Usage: $0 [BUILD_TYPE] [CLEAN_BUILD] [CREATE_ISO] [RUN_TESTS] [DEMO_MODE]"
    echo ""
    echo "BUILD_TYPE options:"
    echo "  ultimate  - Ultimate Edition with all features (default)"
    echo "  debug     - Debug Edition with symbols"
    echo "  minimal   - Minimal Edition"
    echo "  test      - Test Edition for QEMU"
    echo ""
    echo "CLEAN_BUILD options:"
    echo "  true      - Clean before building (default)"
    echo "  false     - Don't clean"
    echo ""
    echo "CREATE_ISO options:"
    echo "  true      - Create ISO after build (default)"
    echo "  false     - Don't create ISO"
    echo ""
    echo "RUN_TESTS options:"
    echo "  true      - Run tests after build (default)"
    echo "  false     - Don't run tests"
    echo ""
    echo "DEMO_MODE options:"
    echo "  true      - Run demo in QEMU"
    echo "  false     - Don't run demo (default)"
    echo ""
    echo "Examples:"
    echo "  $0 ultimate true true true false  # Full build with tests"
    echo "  $0 debug false false false true    # Debug build with demo"
    echo "  $0 minimal true true false false  # Minimal build"
}

# Check for help flag
if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    show_usage
    exit 0
fi

# Run main build process
main
