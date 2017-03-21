#tui enable
target remote :3333  
monitor arm semihosting enable 
monitor init
monitor reset init
monitor halt
load
