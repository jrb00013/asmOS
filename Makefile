CC = gcc
AS = nasm
CFLAGS = -Wall -O2 -g -ffreestanding -nostdlib -fno-builtin
LDFLAGS = -T linker.ld -ffreestanding -nostdlib

# Files
SRC_C = kernel.c memory_manager.c shell.c scheduler.c
SRC_ASM = boot.asm syscalls.asm
OBJ_C = $(SRC_C:.c=.o)
OBJ_ASM = $(SRC_ASM:.asm=.o)
EXEC = os.bin

# Compilation rules
all: $(EXEC)

$(EXEC): $(OBJ_C) $(OBJ_ASM)
    $(CC) $(OBJ_C) $(OBJ_ASM) $(LDFLAGS) -o $(EXEC)
    # Add bootloader and kernel binary merging
    cat boot.asm kernel.bin > os.bin

%.o: %.c
    $(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm
    $(AS) -f elf32 $< -o $@

clean:
    rm -f $(OBJ_C) $(OBJ_ASM) $(EXEC)
