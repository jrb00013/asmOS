# Compiler and Assembler
CC = i686-elf-gcc
AS = nasm
LD = /usr/local/Cellar/i686-elf-binutils/2.43.1/i686-elf/bin/ld
OBJCOPY = i686-elf-objcopy

# Tools
DD = dd
MKFS = mkfs.fat
MCOPY = mcopy

# Compilation Flags !!!!! IN ORDER TO ENABLE OPTIMIZATION FOR DEPLOYMENT
# ..... YOU NEED TO CHANGE -O0 to -O2.      -O0 = debugging    -O2 = optimized deploy
CFLAGS = -m32 -Wall -O0 -g -ffreestanding -nostdlib -fno-builtin -Iinclude
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

.PHONY: all clean run debug gdb

all: $(OS_IMAGE)

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

# Disk image setup
$(OS_IMAGE): $(BOOTLOADER_BIN) $(KERNEL_BIN)
	@mkdir -p $(DISK_DIR)
	$(DD) if=/dev/zero of=$@ bs=512 count=2880
	$(MKFS) -F12 $@
	$(DD) if=$(BOOTLOADER_BIN) of=$@ conv=notrunc
	$(MCOPY) -i $@ $(KERNEL_BIN) ::KERNEL.BIN

# QEMU run
run: $(OS_IMAGE)
	qemu-system-i386 -monitor stdio -drive format=raw,file=$<

# QEMU + GDB debug server
debug: $(KERNEL_ELF)
	qemu-system-i386 -s -S -monitor stdio -drive format=raw,file=$(OS_IMAGE)

# GDB connect
gdb: $(KERNEL_ELF)
	i686-elf-gdb $< -ex "target remote localhost:1234"

clean:
	rm -rf $(BUILD_DIR) $(DISK_DIR)
