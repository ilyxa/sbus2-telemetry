DEVICE          = stm32f303cct6
OPENCM3_DIR     = libopencm3
BINARY			:= sbus-f3-new
OBJS            += $(BINARY).o

include $(OPENCM3_DIR)/mk/gcc-config.mk
include $(OPENCM3_DIR)/mk/genlink-config.mk

CPPFLAGS	+= -I$(OPENCM3_DIR)/include $(DEFS) -MD
CFLAGS		+= -std=gnu99 -O3 -Wall -I. -Werror
CFLAGS		+= -ggdb3
CFLAGS		+= -ffunction-sections -fdata-sections
LDFLAGS		+= --static -nostartfiles -Wl,-Map=$(*).map -L$(OPENCM3_DIR)/lib
LDFLAGS		+= --specs=nano.specs
LDFLAGS 	+= -Wl,-gc-sections
LDLIBS		+= -lc -lgcc



all: $(BINARY).elf $(BINARY).hex

.PHONY: clean all

clean:
	$(Q)$(RM) $(BINARY).d $(BINARY).o $(BINARY).hex $(BINARY).elf $(DEVICE).ld generated.$(DEVICE).ld $(BINARY).bin
bin:
	arm-none-eabi-objcopy -O binary $(BINARY).elf $(BINARY).bin

flash: $(BINARY).elf
	st-util -v0 2>&1 >/dev/null & echo -e "\ntarget remote localhost:4242\nfile $(BINARY).elf\nload\nquit\n" | arm-none-eabi-gdb --quiet | wait `pgrep arm-none-eabi-gdb`

ifeq ($(ENABLE_SEMIHOSTING),1)
  $(info Enabled semihosting)
  LDFLAGS	+= --specs=rdimon.specs
  LDLIBS	+= -lrdimon
  CFLAGS	+= -DSEMIHOSTING=1
else
  LDLIBS	+= -lnosys
endif

include $(OPENCM3_DIR)/mk/genlink-rules.mk
include $(OPENCM3_DIR)/mk/gcc-rules.mk
-include $(OBJS:.o=.d)
