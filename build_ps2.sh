#!/bin/bash

# PS2 x86 OS Build Script v2.0
# Enhanced build script for PlayStation 2 compatibility

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Build configuration
BUILD_TYPE=${1:-"ps2"}
CLEAN_BUILD=${2:-"false"}
CREATE_ISO=${3:-"true"}

echo -e "${BLUE}PS2 x86 OS Build Script v2.0${NC}"
echo -e "${BLUE}============================${NC}"
echo ""

# Function to print colored output
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check dependencies
check_dependencies() {
    print_status "Checking build dependencies..."
    
    local missing_deps=()
    
    # Check for required tools
    command -v nasm >/dev/null 2>&1 || missing_deps+=("nasm")
    command -v i686-elf-gcc >/dev/null 2>&1 || missing_deps+=("i686-elf-gcc")
    command -v make >/dev/null 2>&1 || missing_deps+=("make")
    command -v dd >/dev/null 2>&1 || missing_deps+=("dd")
    command -v mkisofs >/dev/null 2>&1 || missing_deps+=("mkisofs")
    
    if [ ${#missing_deps[@]} -ne 0 ]; then
        print_error "Missing dependencies: ${missing_deps[*]}"
        echo ""
        echo "Install missing dependencies:"
        echo "sudo apt-get install nasm gcc-multilib gcc-multilib-i686-elf make mkisofs"
        exit 1
    fi
    
    print_status "All dependencies found!"
}

# Clean previous builds
clean_build() {
    if [ "$CLEAN_BUILD" = "true" ]; then
        print_status "Cleaning previous build artifacts..."
        make clean
        print_status "Build artifacts cleaned!"
    fi
}

# Build the OS
build_os() {
    print_status "Building PS2 x86 OS..."
    
    case $BUILD_TYPE in
        "ps2")
            print_status "Building with PS2 optimizations..."
            make ps2-build
            ;;
        "debug")
            print_status "Building with debug information..."
            make all
            ;;
        "test")
            print_status "Building for testing..."
            make ps2-test
            ;;
        *)
            print_warning "Unknown build type: $BUILD_TYPE"
            print_status "Using default PS2 build..."
            make ps2-build
            ;;
    esac
    
    print_status "Build completed successfully!"
}

# Create ISO image
create_iso() {
    if [ "$CREATE_ISO" = "true" ]; then
        print_status "Creating bootable ISO image..."
        
        if [ -f "disk/os.img" ]; then
            make iso
            print_status "ISO created: ps2os.iso"
            
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

# Show build information
show_build_info() {
    echo ""
    echo -e "${BLUE}Build Information:${NC}"
    echo "=================="
    echo "Build Type: $BUILD_TYPE"
    echo "Clean Build: $CLEAN_BUILD"
    echo "Create ISO: $CREATE_ISO"
    echo "Target: PS2 x86 Assembly Real-Mode OS"
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

# Show next steps
show_next_steps() {
    echo ""
    echo -e "${BLUE}Next Steps:${NC}"
    echo "============"
    
    if [ -f "ps2os.iso" ]; then
        echo "1. Burn ISO to CD:"
        echo "   growisofs -dvd-compat -Z /dev/sr0=ps2os.iso"
        echo ""
        echo "2. Boot on PS2:"
        echo "   - Insert CD into PS2"
        echo "   - Power on with modchip enabled"
        echo "   - Use USB keyboard for interaction"
    else
        echo "1. Test in QEMU:"
        echo "   make ps2-test"
        echo ""
        echo "2. Create ISO:"
        echo "   make iso"
    fi
    
    echo ""
    echo "For more information, see README.md"
}

# Main build process
main() {
    echo "Starting PS2 x86 OS build process..."
    echo ""
    
    # Check dependencies
    check_dependencies
    
    # Clean if requested
    clean_build
    
    # Build the OS
    build_os
    
    # Create ISO if requested
    create_iso
    
    # Show build information
    show_build_info
    
    # Show next steps
    show_next_steps
    
    echo ""
    print_status "Build process completed successfully!"
}

# Show usage
show_usage() {
    echo "Usage: $0 [BUILD_TYPE] [CLEAN_BUILD] [CREATE_ISO]"
    echo ""
    echo "BUILD_TYPE options:"
    echo "  ps2    - PS2 optimized build (default)"
    echo "  debug  - Debug build with symbols"
    echo "  test   - Test build for QEMU"
    echo ""
    echo "CLEAN_BUILD options:"
    echo "  true   - Clean before building"
    echo "  false  - Don't clean (default)"
    echo ""
    echo "CREATE_ISO options:"
    echo "  true   - Create ISO after build (default)"
    echo "  false  - Don't create ISO"
    echo ""
    echo "Examples:"
    echo "  $0                    # PS2 build with ISO"
    echo "  $0 debug true false   # Debug build, clean, no ISO"
    echo "  $0 test false true     # Test build, no clean, with ISO"
}

# Check for help flag
if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    show_usage
    exit 0
fi

# Run main build process
main
