DEVICE          = stm32f303cct6
OPENCM3_DIR     = libopencm3
BINARY		:= sbus-f3-new
OBJS            += $(BINARY).o

CFLAGS          += -Os -ggdb3 -std=gnu99 -O3 -Wall -I. -Werror
CPPFLAGS        += -MD
LDFLAGS         += -static -nostartfiles
LDLIBS          += -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group

include $(OPENCM3_DIR)/mk/genlink-config.mk
include $(OPENCM3_DIR)/mk/gcc-config.mk

.PHONY: clean all

all: $(BINARY).elf $(BINARY).hex

clean:
	$(Q)$(RM) $(BINARY).d $(BINARY).hex $(BINARY).elf $(DEVICE).ld generated.$(DEVICE).ld

flash: $(BINARY).elf
	st-util -v0 2>&1 >/dev/null & echo -e "\ntarget remote localhost:4242\nfile sbus.elf\nload\nquit\n" | arm-none-eabi-gdb --quiet | wait `pgrep arm-none-eabi-gdb`

include $(OPENCM3_DIR)/mk/genlink-rules.mk
include $(OPENCM3_DIR)/mk/gcc-rules.mk
