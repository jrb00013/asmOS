# Compiler and Assembler
CC = gcc
AS = nasm
LD = ld

# Compilation Flags
CFLAGS = -m32 -Wall -O2 -g -ffreestanding -nostdlib -fno-builtin
LDFLAGS = -T linker/linker.ld

# Directories
SRC_DIR = src
BOOT_DIR = boot
BUILD_DIR = build

# Source Files
SRC_C = $(wildcard $(SRC_DIR)/*.c)
SRC_ASM = $(wildcard $(BOOT_DIR)/*.asm)

# Object Files
OBJ_C = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRC_C))
OBJ_ASM = $(patsubst $(BOOT_DIR)/%.asm, $(BUILD_DIR)/%.o, $(SRC_ASM))
OBJS = $(OBJ_C) $(OBJ_ASM)

# Output File
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
OS_BIN = $(BUILD_DIR)/os.bin

# Rules
all: $(OS_BIN)

# Compile C source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble Assembly source files
$(BUILD_DIR)/%.o: $(BOOT_DIR)/%.asm
	@mkdir -p $(BUILD_DIR)
	$(AS) -f elf32 $< -o $@

# Link everything
$(KERNEL_BIN): $(OBJS)
	$(LD) $(LDFLAGS) -o $@

# Merge bootloader and kernel
$(OS_BIN): $(KERNEL_BIN)
	cat $(BOOT_DIR)/boot.asm $(KERNEL_BIN) > $(OS_BIN)

# Clean build files
clean:
	rm -rf $(BUILD_DIR)
