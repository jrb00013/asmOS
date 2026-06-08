# ASMOS PS2 native build (PS2SDK / ee-gcc)
ROOT := ../..
PS2DEV ?= /usr/local/ps2dev
PS2SDK ?= $(PS2DEV)/ps2sdk

ifeq (,$(wildcard $(PS2SDK)/samples/Makefile.eeglobal))
$(error PS2SDK not found at $(PS2SDK). Install ps2dev or run scripts/build_ps2_native.sh)
endif

EE_BIN = $(ROOT)/build/asmos.elf
EE_OBJS_DIR = $(ROOT)/build/ps2/

SHARED_SRC := $(wildcard $(ROOT)/src/*.c) $(wildcard $(ROOT)/src/subsys/*.c) $(wildcard $(ROOT)/src/quantum/*.c)
SHARED_SRC := $(filter-out $(ROOT)/src/vga.c $(ROOT)/src/keyboard.c,$(SHARED_SRC))
PS2_SRC := main.c kernel_console.c stubs.c io_syscalls.c iop_init.c \
           hal_system.c hal_input.c hal_storage.c hal_net.c hal_video.c

EE_CFLAGS += -DPLATFORM_PS2=1 -DPS2_HARDWARE=1 -I$(ROOT)/include
EE_CXXFLAGS += $(EE_CFLAGS)
EE_LDFLAGS += -Wl,-Map=$(ROOT)/build/asmos.map
EE_LIBS := -lpad -lm -lc -lmc -lfileXio -ldebug -lnetman -lps2ip -lpacket \
           -ldraw -lgraph -ldma -laudio -lutil

EE_OBJS := $(patsubst $(ROOT)/src/%.c,$(EE_OBJS_DIR)src/%.o,$(SHARED_SRC))
EE_OBJS += $(EE_OBJS_DIR)src/net/udp.o $(EE_OBJS_DIR)src/net_clients.o
EE_OBJS += $(addprefix $(EE_OBJS_DIR)ps2/,$(PS2_SRC:.c=.o))

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal

$(EE_OBJS_DIR) $(EE_OBJS_DIR)src/net $(EE_OBJS_DIR)ps2:
	@mkdir -p $(EE_OBJS_DIR) $(EE_OBJS_DIR)src/net $(EE_OBJS_DIR)ps2

$(EE_OBJS_DIR)src/%.o: $(ROOT)/src/%.c | $(EE_OBJS_DIR)
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@

$(EE_OBJS_DIR)src/net/%.o: $(ROOT)/src/net/%.c | $(EE_OBJS_DIR)
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@

$(EE_OBJS_DIR)src/net_clients.o: $(ROOT)/src/net_clients.c | $(EE_OBJS_DIR)
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@

$(EE_OBJS_DIR)ps2/%.o: %.c | $(EE_OBJS_DIR)
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@

clean:
	rm -rf $(EE_OBJS_DIR) $(EE_BIN) $(ROOT)/build/asmos.map

.PHONY: clean
