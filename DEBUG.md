openocd  -f interface/stlink-v2.cfg -f target/stm32f3x.cfg


arm-none-eabi-gdb sbus-f3-u1.elf
target remote :3333  
monitor arm semihosting enable 
monitor init
monitor reset init
load
continue


delete - delete all breakpoints

break sbus-f3-new.c:137

read here https://zhevak.wordpress.com/2016/11/03/%D1%81%D0%BF%D0%B8%D1%81%D0%BE%D0%BA-%D0%BD%D0%B0%D0%B8%D0%B1%D0%BE%D0%BB%D0%B5%D0%B5-%D0%B8%D1%81%D0%BF%D0%BE%D0%BB%D1%8C%D0%B7%D1%83%D0%B5%D0%BC%D1%8B%D1%85-%D0%BA%D0%BE%D0%BC%D0%B0%D0%BD%D0%B4-gdb/#more-5786
