# ASMOS v3.0 Makefile
CC = gcc
AS = nasm
LD = ld
OBJCOPY = objcopy

QEMU = $(shell which qemu-system-i386 2>/dev/null || which qemu-system-x86_64 2>/dev/null || echo qemu-system-i386)
MKISOFS = $(shell which mkisofs 2>/dev/null || which genisoimage 2>/dev/null || echo mkisofs)
MKFS = $(shell which mkfs.fat 2>/dev/null || which mkdosfs 2>/dev/null || echo mkfs.fat)
MCOPY = $(shell which mcopy 2>/dev/null || echo mcopy)
MTOOLS_MKE2F = 1
DD = dd

CFLAGS = -m32 -Wall -O2 -g -ffreestanding -nostdlib -fno-builtin -Iinclude -march=i686 -mtune=pentium3 -fno-stack-protector -Wno-unused-variable -Wno-unused-function -Wno-unused-parameter -Wno-unused-label
ASFLAGS = -f elf32 -w+other
LDFLAGS = -m elf_i386 -T llinker/linker.ld --gc-sections

SRC_DIR = src
BOOT_DIR = boot
BUILD_DIR = build
DISK_DIR = disk
TOOLS_DIR = tools

KERNEL_C = $(filter-out $(SRC_DIR)/block_dev.c $(SRC_DIR)/fat12.c, \
           $(wildcard $(SRC_DIR)/*.c) $(wildcard platform/x86/*.c) $(wildcard $(SRC_DIR)/net/*.c) \
           $(wildcard $(SRC_DIR)/quantum/*.c) $(wildcard $(SRC_DIR)/subsys/*.c))
BOOT_ASM_BIN = boot.asm bootsect.asm loader.asm stage1.asm fatload16.asm image.asm gdt.asm pm.asm rm_thunk.asm stage2_entry.asm
SRC_ASM = $(filter-out $(addprefix $(BOOT_DIR)/,$(BOOT_ASM_BIN)), $(wildcard $(BOOT_DIR)/*.asm))

OBJ_C = $(patsubst %.c,$(BUILD_DIR)/%.o,$(KERNEL_C))
OBJ_ASM = $(patsubst $(BOOT_DIR)/%.asm,$(BUILD_DIR)/%.o,$(SRC_ASM))
OBJS = $(OBJ_C) $(OBJ_ASM)

BOOTSECT_BIN = $(BUILD_DIR)/bootsect.bin
LOADER_BIN = $(BUILD_DIR)/loader.bin
STAGE1_BIN = $(BUILD_DIR)/stage1.bin
KERNEL_ELF = $(BUILD_DIR)/kernel.elf
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
BOOT_META = $(BUILD_DIR)/ASMOS.META
OS_IMAGE = $(DISK_DIR)/os.img

.PHONY: all clean run debug test-integration fmcb-package setup ps2-native commit-wave

CFLAGS += -DPLATFORM_X86=1

all: $(OS_IMAGE)

$(BOOTSECT_BIN): $(BOOT_DIR)/bootsect.asm
	@mkdir -p $(BUILD_DIR)
	@echo "  AS    bootsect"
	@$(AS) -f bin $< -o $@

$(LOADER_BIN): $(BOOT_DIR)/loader.asm
	@echo "  AS    loader"
	@$(AS) -f bin $< -o $@

$(STAGE1_BIN): $(BOOTSECT_BIN) $(LOADER_BIN)
	@cp $(BOOTSECT_BIN) $@
	@dd if=$(LOADER_BIN) of=$@ bs=512 seek=1 conv=notrunc status=none
	@dd if=/dev/zero of=$@ bs=512 seek=$$(($(shell wc -c < $(LOADER_BIN))/512 + 1)) count=$$((32 - $(shell wc -c < $(LOADER_BIN))/512)) conv=notrunc status=none 2>/dev/null || true

$(BUILD_DIR)/tools/gen_boot_meta: $(TOOLS_DIR)/gen_boot_meta.c include/boot_meta.h
	@mkdir -p $(BUILD_DIR)/tools
	@$(CC) -Iinclude -o $@ $<

$(BOOT_META): $(KERNEL_BIN) $(BUILD_DIR)/tools/gen_boot_meta
	@$(BUILD_DIR)/tools/gen_boot_meta $(KERNEL_BIN) $@ x86-qemu

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "  CC    $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(BOOT_DIR)/%.asm
	@mkdir -p $(BUILD_DIR)
	@echo "  AS    $<"
	@$(AS) $(ASFLAGS) $< -o $@

$(KERNEL_ELF): $(OBJS)
	@echo "  LD    kernel.elf"
	@$(LD) $(LDFLAGS) -o $@ $^

$(KERNEL_BIN): $(KERNEL_ELF)
	@echo "  OBJCOPY kernel.bin"
	@$(OBJCOPY) -O binary $< $@

$(OS_IMAGE): $(STAGE1_BIN) $(KERNEL_BIN) $(BOOT_META)
	@mkdir -p $(DISK_DIR)
	@echo "Creating FAT12 disk image with KERNEL.BIN..."
	$(DD) if=/dev/zero of=$@ bs=512 count=2880 status=none
	$(MKFS) -F 12 -n PS2OS -i 0x12345678 $@
	@bash scripts/fat_put.sh $@ $(KERNEL_BIN) "KERNEL  BIN"
	@bash scripts/fat_put.sh $@ $(BOOT_META) "ASMOS   META"
	$(DD) if=$(STAGE1_BIN) of=$@ conv=notrunc status=none
	@if [ -f boot/CONFIG.WF ]; then bash scripts/fat_put.sh $@ boot/CONFIG.WF "CONFIG  WF  "; fi
	@echo "Disk image: $@ ($(shell stat -c%s $@) bytes)"

run: $(OS_IMAGE)
	$(QEMU) -drive format=raw,file=$< -m 32 -serial stdio

clean:
	rm -rf $(BUILD_DIR) $(DISK_DIR) ps2os.iso

test-integration:
	@bash tests/integration/run_checks.sh

ps2-native:
	@bash scripts/build_ps2_native.sh

fmcb-package: ps2-native
	@bash scripts/build_fmcb_package.sh

setup:
	@bash setup.sh

info:
	@echo "ASMOS v3.0 — load addr 0x100000, FAT12 KERNEL.BIN"
