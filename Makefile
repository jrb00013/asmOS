# Compiler and Assembler
CC = i686-elf-gcc
AS = nasm
LD = /usr/local/Cellar/i686-elf-binutils/2.43.1/i686-elf/bin/ld

# Tools
DD = dd
MKFS = mkfs.fat
MCOPY = mcopy
OBJCOPY = i686-elf-objcopy

# Compilation Flags
CFLAGS = -m32 -Wall -O2 -g -ffreestanding -nostdlib -fno-builtin -Iinclude
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T llinker/linker.ld --gc-sections

# Directories
SRC_DIR = src
BOOT_DIR = boot
INCLUDE_DIR = include
BUILD_DIR = build
DISK_DIR = disk

# Source Files
SRC_C = $(wildcard $(SRC_DIR)/*.c)
SRC_ASM = $(filter-out $(BOOT_DIR)/boot.asm, $(wildcard $(BOOT_DIR)/*.asm))

# Object Files
OBJ_C = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRC_C))
OBJ_ASM = $(patsubst $(BOOT_DIR)/%.asm, $(BUILD_DIR)/%.o, $(SRC_ASM))
OBJS = $(OBJ_C) $(OBJ_ASM)

# Output Files
BOOTLOADER_BIN = $(BUILD_DIR)/boot.bin
KERNEL_ELF = $(BUILD_DIR)/kernel.elf
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
OS_IMAGE = $(DISK_DIR)/os.img

.PHONY: all clean run

all: $(OS_IMAGE)

# Build bootloader as raw binary
$(BOOTLOADER_BIN): $(BOOT_DIR)/boot.asm
	@mkdir -p $(BUILD_DIR)
	$(AS) -f bin -I$(BOOT_DIR)/ $< -o $@

# Compile C source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble ASM files (as ELF)
$(BUILD_DIR)/%.o: $(BOOT_DIR)/%.asm
	@mkdir -p $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@

# Link kernel
$(KERNEL_ELF): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

# Create flat binary from ELF
$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $< $@

# Create disk image
$(OS_IMAGE): $(BOOTLOADER_BIN) $(KERNEL_BIN)
	@mkdir -p $(DISK_DIR)
	$(DD) if=/dev/zero of=$@ bs=512 count=2880
	$(MKFS) -F12 $@
	$(DD) if=$(BOOTLOADER_BIN) of=$@ conv=notrunc
	$(MCOPY) -i $@ $(KERNEL_BIN) ::kernel.bin

# Run in QEMU
run: $(OS_IMAGE)
	qemu-system-i386 -monitor stdio -drive format=raw,file=$<

clean:
	rm -rf $(BUILD_DIR) $(DISK_DIR)