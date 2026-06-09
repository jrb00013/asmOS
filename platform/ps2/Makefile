# ASMOS PS2 native build (PS2SDK / ee-gcc)
ROOT := ../..
PS2DEV ?= /usr/local/ps2dev
PS2SDK ?= $(PS2DEV)/ps2sdk

ifeq (,$(wildcard $(PS2SDK)/samples/Makefile.eeglobal))
$(error PS2SDK not found at $(PS2SDK). Install ps2dev or run scripts/build_ps2_native.sh)
endif

EE_BIN = $(ROOT)/build/asmos.elf
EE_OBJS_DIR = $(ROOT)/build/ps2/

FILTERED := $(ROOT)/src/vga.c \
	$(ROOT)/src/block_dev.c $(ROOT)/src/fat12.c $(ROOT)/src/kernel_start.c \
	$(ROOT)/src/debugcon.c
PS2_SRC := main.c kernel_console.c stubs.c io_syscalls.c iop_init.c \
           hal_system.c hal_input.c hal_storage.c hal_net.c hal_video.c

SHARED_TOP := $(filter-out $(FILTERED),$(wildcard $(ROOT)/src/*.c))
SHARED_SUBSYS := $(wildcard $(ROOT)/src/subsys/*.c)
SHARED_QUANTUM := $(wildcard $(ROOT)/src/quantum/*.c)

EE_OBJS := $(patsubst $(ROOT)/src/%.c,$(EE_OBJS_DIR)src/%.o,$(SHARED_TOP))
EE_OBJS += $(patsubst $(ROOT)/src/subsys/%.c,$(EE_OBJS_DIR)src/subsys/%.o,$(SHARED_SUBSYS))
EE_OBJS += $(patsubst $(ROOT)/src/quantum/%.c,$(EE_OBJS_DIR)src/quantum/%.o,$(SHARED_QUANTUM))
EE_OBJS += $(EE_OBJS_DIR)src/net/udp.o $(EE_OBJS_DIR)src/net/icmp.o
EE_OBJS += $(addprefix $(EE_OBJS_DIR)ps2/,$(PS2_SRC:.c=.o))

EE_LDFLAGS += -Wl,-Map=$(ROOT)/build/asmos.map
EE_LIBS := -lpad -lm -lc -lmc -lfileXio -ldebug -lnetman -lps2ip -lpacket \
           -ldraw -lgraph -ldma

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal

EE_ASMOS_INCS := -I$(ROOT)/include
EE_CFLAGS += -DPLATFORM_PS2=1 -DPS2_HARDWARE=1

$(EE_OBJS_DIR) $(EE_OBJS_DIR)src/net $(EE_OBJS_DIR)src/subsys $(EE_OBJS_DIR)src/quantum $(EE_OBJS_DIR)ps2:
	@mkdir -p $(EE_OBJS_DIR) $(EE_OBJS_DIR)src/net $(EE_OBJS_DIR)src/subsys $(EE_OBJS_DIR)src/quantum $(EE_OBJS_DIR)ps2

$(EE_OBJS_DIR)src/%.o: $(ROOT)/src/%.c | $(EE_OBJS_DIR)
	$(EE_CC) $(EE_CFLAGS) $(EE_ASMOS_INCS) $(EE_INCS) -c $< -o $@

$(EE_OBJS_DIR)src/net/%.o: $(ROOT)/src/net/%.c | $(EE_OBJS_DIR)
	$(EE_CC) $(EE_CFLAGS) $(EE_ASMOS_INCS) $(EE_INCS) -c $< -o $@

$(EE_OBJS_DIR)src/net_clients.o: $(ROOT)/src/net_clients.c | $(EE_OBJS_DIR)
	$(EE_CC) $(EE_CFLAGS) $(EE_ASMOS_INCS) $(EE_INCS) -c $< -o $@

$(EE_OBJS_DIR)src/subsys/%.o: $(ROOT)/src/subsys/%.c | $(EE_OBJS_DIR)
	$(EE_CC) $(EE_CFLAGS) $(EE_ASMOS_INCS) $(EE_INCS) -c $< -o $@

$(EE_OBJS_DIR)src/quantum/%.o: $(ROOT)/src/quantum/%.c | $(EE_OBJS_DIR)
	$(EE_CC) $(EE_CFLAGS) $(EE_ASMOS_INCS) $(EE_INCS) -c $< -o $@

$(EE_OBJS_DIR)ps2/%.o: %.c | $(EE_OBJS_DIR)
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) $(EE_ASMOS_INCS) -c $< -o $@

clean:
	rm -rf $(EE_OBJS_DIR) $(EE_BIN) $(ROOT)/build/asmos.map

.PHONY: clean
