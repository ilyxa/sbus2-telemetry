# sbus2-telemetry
Futaba Sbus2 telemetry tryout

# Setup (Arch Linux)
community/stlink 1.2.0-2

community/arm-none-eabi-gcc 6.1.1-4

community/arm-none-eabi-gdb 7.11.1-1

# Preparing
git submodule update --init # do not forget this, do not try compile

make -C libopencm3 #-j16

# Running
make

make flash

# TBD
