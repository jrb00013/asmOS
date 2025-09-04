# Enhanced PS2 x86 OS Makefile v3.0
# Compiler and Assembler
CC = gcc
AS = nasm
LD = ld
OBJCOPY = objcopy

# Tools with fallbacks
QEMU = $(shell which qemu-system-i386 2>/dev/null || which qemu-system-x86_64 2>/dev/null || echo qemu-system-i386)
MKISOFS = $(shell which mkisofs 2>/dev/null || which genisoimage 2>/dev/null || echo mkisofs)
GROWISOFS = $(shell which growisofs 2>/dev/null || echo growisofs)
MKFS = $(shell which mkfs.fat 2>/dev/null || which mkdosfs 2>/dev/null || echo mkfs.fat)
DD = dd
MCOPY = mcopy

# Enhanced Compilation Flags for PS2 optimization
# -O2 for production optimization
# -march=i686 for PS2-compatible x86 instructions
# -mtune=pentium3 for PS2-era CPU optimization
# -fno-stack-protector for better compatibility
CFLAGS = -m32 -Wall -O2 -g -ffreestanding -nostdlib -fno-builtin -Iinclude -march=i686 -mtune=pentium3 -fno-stack-protector
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T llinker/linker.ld --gc-sections

# Directories
SRC_DIR = src
BOOT_DIR = boot
INCLUDE_DIR = include
BUILD_DIR = build
DISK_DIR = disk

# Files
SRC_C = $(wildcard $(SRC_DIR)/*.c)
SRC_ASM = $(filter-out $(BOOT_DIR)/boot.asm, $(wildcard $(BOOT_DIR)/*.asm))

OBJ_C = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRC_C))
OBJ_ASM = $(patsubst $(BOOT_DIR)/%.asm, $(BUILD_DIR)/%.o, $(SRC_ASM))
OBJS = $(OBJ_C) $(OBJ_ASM)

BOOTLOADER_BIN = $(BUILD_DIR)/boot.bin
KERNEL_ELF = $(BUILD_DIR)/kernel.elf
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
OS_IMAGE = $(DISK_DIR)/os.img

.PHONY: all clean run debug gdb ps2-build ps2-test

all: $(OS_IMAGE)

# PS2-specific build target
ps2-build: CFLAGS += -DPS2_HARDWARE=1
ps2-build: $(OS_IMAGE)
	@echo "PS2-optimized build complete!"
	@echo "Ready for CD burning with: growisofs -dvd-compat -Z /dev/sr0=disk/os.img"

# Bootloader binary (raw)
$(BOOTLOADER_BIN): $(BOOT_DIR)/boot.asm
	@mkdir -p $(BUILD_DIR)
	$(AS) -f bin -I$(BOOT_DIR)/ $< -o $@

# Compile C sources
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble any additional ASM as ELF
$(BUILD_DIR)/%.o: $(BOOT_DIR)/%.asm
	@mkdir -p $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@

# Link kernel
$(KERNEL_ELF): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

# Convert ELF to flat binary
$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) -O binary --set-start 0x1000 $< $@

# Enhanced disk image setup for PS2
$(OS_IMAGE): $(BOOTLOADER_BIN) $(KERNEL_BIN)
	@mkdir -p $(DISK_DIR)
	@echo "Creating PS2-compatible disk image..."
	$(DD) if=/dev/zero of=$@ bs=512 count=2880
	$(MKFS) -F12 $@
	$(DD) if=$(BOOTLOADER_BIN) of=$@ conv=notrunc
	$(DD) if=$(KERNEL_BIN) of=$@ bs=512 seek=1 conv=notrunc
	@echo "Disk image created: $@"
	@echo "Size: $(shell stat -c%s $@) bytes"

# QEMU run (for testing)
run: $(OS_IMAGE)
	$(QEMU) -monitor stdio -drive format=raw,file=$< -m 32

# QEMU + GDB debug server
debug: $(KERNEL_ELF)
	$(QEMU) -s -S -monitor stdio -drive format=raw,file=$(OS_IMAGE) -m 32

# GDB connect
gdb: $(KERNEL_ELF)
	i686-elf-gdb $< -ex "target remote localhost:1234"

# PS2 test build (with PS2-specific optimizations)
ps2-test: ps2-build
	@echo "Testing PS2 build in QEMU..."
	$(QEMU) -monitor stdio -drive format=raw,file=$(OS_IMAGE) -m 32 -cpu pentium3

# Create ISO for CD burning
iso: $(OS_IMAGE)
	@echo "Creating ISO image for CD burning..."
	$(MKISOFS) -o ps2os.iso -b os.img -no-emul-boot -boot-load-size 4 -boot-info-table $(DISK_DIR)
	@echo "ISO created: ps2os.iso"
	@echo "Ready for burning with: $(GROWISOFS) -dvd-compat -Z /dev/sr0=ps2os.iso"

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(DISK_DIR) ps2os.iso

# Show build information
info:
	@echo "PS2 x86 OS Build Information:"
	@echo "============================="
	@echo "Compiler: $(CC)"
	@echo "Assembler: $(AS)"
	@echo "Linker: $(LD)"
	@echo "CFLAGS: $(CFLAGS)"
	@echo "Target: PS2 x86 Assembly Real-Mode OS"
	@echo "Boot Method: Modchip (DMS3)"
	@echo "Media: CD/DVD"
	@echo "Requirements: PS2, DMS3 Modchip, Linux Terminal, NASM, GCC"
