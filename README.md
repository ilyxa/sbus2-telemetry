# sbus2-telemetry
Futaba Sbus2 telemetry tryout.

This is very dirty prototype fork from original project https://github.com/delamonpansie/jitel for STM32F3 MCU.
# Setup (Arch Linux)
community/stlink

community/arm-none-eabi-gcc

community/arm-none-eabi-gdb

community/openocd
# Preparing
git submodule update --init # do not forget this, do not try compile

make -C libopencm3 #-j16

# Running
make

make flash
# Debug
openocd  -f interface/stlink-v2.cfg -f target/stm32f3x.cfg &

Check .gdbinit for options before up runnning

Also install newlib if u want swd printout (why??)

arm-none-eabi-gdb

# TBD
