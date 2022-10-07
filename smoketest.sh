#!/bin/bash
export ARCH=x86
export INCLUDE_DEMO=1
make clean
make

echo "Launching QEMU"
qemu-system-i386 \
	-serial tcp:127.0.0.1:1234,server \
	-display none \
	-kernel gdbstub.bin &

cat<<EOF >smoketest.gdbinit
set pagination off
set tcp connect-timeout 2
symbol-file gdbstub.elf
target remote 127.0.0.1:1234
b simple_loop
c

# Step twice and check value of variable x
s 9
p/x x
if x == 0xdeadbee0
	printf "PASS\n"
	quit 0
else
	printf "FAIL\n"
	quit 1
end
EOF

echo "Running GDB"
gdb --batch --command smoketest.gdbinit
RESULT=$?

echo "Terminating QEMU"
killall qemu-system-i386

exit $RESULT
