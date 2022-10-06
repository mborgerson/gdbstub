# To observe RSP packets, un-comment the following line:
# set debug remote 1

symbol-file gdbstub-x86.elf
target remote 127.0.0.1:1234
b simple_loop
layout split
c
