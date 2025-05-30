# Compiler and Assembler
CC = gcc
AS = nasm
LD = ld

# Tools
DD = dd
MKFS = mkfs.fat
MCOPY = mcopy

# Compilation Flags
CFLAGS = -m32 -Wall -O2 -g -ffreestanding -nostdlib -fno-builtin -Iinclude
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T linker.ld

# Directories
SRC_DIR = src
BOOT_DIR = boot
INCLUDE_DIR = include
BUILD_DIR = build
DISK_DIR = disk

# Source Files
SRC_C = $(wildcard $(SRC_DIR)/*.c)
SRC_ASM = $(wildcard $(BOOT_DIR)/*.asm)

# Object Files
OBJ_C = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRC_C))
OBJ_ASM = $(patsubst $(BOOT_DIR)/%.asm, $(BUILD_DIR)/%.o, $(SRC_ASM))
OBJS = $(OBJ_C) $(OBJ_ASM)

# Output Files
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
BOOTLOADER_BIN = $(BUILD_DIR)/boot.bin
OS_IMAGE = $(DISK_DIR)/os.img

.PHONY: all clean run

all: $(OS_IMAGE)

# Compile C source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble bootloader (as flat binary)
$(BUILD_DIR)/boot.o: $(BOOT_DIR)/boot.asm
	@mkdir -p $(BUILD_DIR)
	$(AS) -f bin $< -o $@

# Assemble other ASM files (as ELF)
$(BUILD_DIR)/%.o: $(BOOT_DIR)/%.asm
	@mkdir -p $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@

# Link kernel
$(KERNEL_BIN): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

# Create disk image
$(OS_IMAGE): $(KERNEL_BIN) $(BUILD_DIR)/boot.o
	@mkdir -p $(DISK_DIR)
	$(DD) if=/dev/zero of=$@ bs=512 count=2880
	$(MKFS) -F12 $@
	$(MCOPY) -i $@ $(KERNEL_BIN) ::kernel.bin
	$(DD) if=$(BUILD_DIR)/boot.o of=$@ conv=notrunc

# Run in QEMU (for testing)
run: $(OS_IMAGE)
	qemu-system-i386 -drive format=raw,file=$<

# Clean build files
clean:
	rm -rf $(BUILD_DIR) $(DISK_DIR)
