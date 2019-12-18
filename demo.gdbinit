# To observe RSP packets, enable the following line:
# set debug remote 1

symbol-file gdbstub.elf
target remote 127.0.0.1:1234
b simple_loop
layout split
c
