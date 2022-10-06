gdbstub
=======
This is a simple GDB stub that can be easily dropped in to your project to allow
you to debug a target platform using GDB (or another application which supports
remote GDB targets). It has no library dependencies (such as libc) and requires
just standard tools to build.

Protocol
--------
Communication between the stub and the debugger takes place via the [GDB
Remote Serial Protocol](https://sourceware.org/gdb/onlinedocs/gdb/Remote-Protocol.html).

Porting
-------
This was developed for x86 systems, but it's fairly modular. With a little
effort, it can be easily ported to other platforms. You will need to modify
`gdbstub_sys.h` and `gdbstub_sys.c` to fit your platform's needs accordingly.

Building
--------
Running `make` produces ELF binary `gdbstub.elf` with an entry point
(`dbg_start`) that will simply hook the current IDT (to support debug
interrupts) and break.

Additionally, a simple flat binary `gdbstub.bin` is created from the ELF binary.
The intent for this flat binary is to be trivially loaded and jumped to.

Demo
----
In `demo/demo.c` there is a simple function that's used for demonstration and
testing. To test the GDB stub out, you can launch an instance of the full-system
emulator [QEMU](https://www.qemu.org/) as follows:

	qemu-system-i386 -serial tcp:127.0.0.1:1234,server -display none -kernel gdbstub.elf

This will launch QEMU, create a virtual machine with a virtual serial port that
can be connected to through local TCP port 1234, then load and run the stub
executable inside the virtual machine.

You can then launch your local GDB client with the `demo.gdbinit` script to get
your GDB client to connect to the virtual serial port and begin debugging the
demo application:

	gdb --command=demo/demo.gdbinit

For example, step a couple of times and print out the value of `x`:

	(gdb) s 2
	(gdb) p/x x
	$1 = 0xdeadbeef

License
-------
This software is published under the terms of the MIT License. See `LICENSE.txt`
for full license.

Matt Borgerson, 2016-2022
