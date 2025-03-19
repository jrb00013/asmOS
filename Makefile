# Compiler and Assembler
CC = gcc
AS = nasm
LD = ld

# Compilation Flags
CFLAGS = -m32 -Wall -O2 -g -ffreestanding -nostdlib -fno-builtin -Iinclude
LDFLAGS = -T linker/linker.ld

# Directories
SRC_DIR = src
BOOT_DIR = boot
INCLUDE_DIR = include
BUILD_DIR = build

# Source Files
SRC_C = $(wildcard $(SRC_DIR)/*.c)
SRC_ASM = $(wildcard $(BOOT_DIR)/*.asm)

# Header Files (for reference, not compiled)
HEADERS = $(wildcard $(INCLUDE_DIR)/*.h)

# Object Files
OBJ_C = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRC_C))
OBJ_ASM = $(patsubst $(BOOT_DIR)/%.asm, $(BUILD_DIR)/%.o, $(SRC_ASM))
OBJS = $(OBJ_C) $(OBJ_ASM)

# Output Files
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
OS_BIN = $(BUILD_DIR)/os.bin

# Rules
all: $(OS_BIN)

# Compile C source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble Assembly source files
$(BUILD_DIR)/%.o: $(BOOT_DIR)/%.asm
	@mkdir -p $(BUILD_DIR)
	$(AS) -f elf32 $< -o $@

# Link all object files into the kernel binary
$(KERNEL_BIN): $(OBJS)
	$(LD) $(LDFLAGS) -o $(KERNEL_BIN) $(OBJS)

# Merge bootloader and kernel
$(OS_BIN): $(KERNEL_BIN) $(BOOT_DIR)/bootloader.bin
	cat $(BOOT_DIR)/bootloader.bin $(KERNEL_BIN) > $(OS_BIN)

# Clean build files
clean:
	rm -rf $(BUILD_DIR)
