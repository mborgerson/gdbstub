set pagination off
set tcp connect-timeout 2
symbol-file gdbstub-x86.elf
target remote 127.0.0.1:1234
b simple_loop
c

# Step twice and check value of `x` variable
s 2
if x == 0xdeadbeef
	printf "PASS\n"
	quit 0
else
	printf "FAIL\n"
	quit 1
end
