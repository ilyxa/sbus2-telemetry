openocd  -f interface/stlink-v2.cfg -f target/stm32f3x.cfg


arm-none-eabi-gdb sbus-f3-u1.elf
target remote :3333  
monitor arm semihosting enable 
monitor init
monitor reset init
load
continue

arm-none-eabi-gdb --iex "target remote :3333" \
	--iex "monitor arm semihosting enable" \
	--iex "monitor init" \
	--iex "monitor reset init" \
	--iex "load" \
	--iex "continue" \
	sbus-f3-u1.elf
