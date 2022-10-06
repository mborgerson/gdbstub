#!/bin/bash
echo "Launching QEMU"
qemu-system-i386 \
	-serial tcp:127.0.0.1:1234,server \
	-display none \
	-kernel gdbstub.bin &

echo "Running GDB"
gdb --batch --command tests/smoketest.gdbinit
RESULT=$?

echo "Terminating QEMU"
killall qemu-system-i386

exit $RESULT
