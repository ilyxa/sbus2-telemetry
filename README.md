#First of all
Many thanks to original project maintainer, https://github.com/delamonpansie/jitel was very nice as start point, also code was partialy copypasted from this project.

# sbus2-telemetry
Futaba Sbus2 telemetry tryout.

Code cleanup wanted, also need to write down right connection scheme.

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
