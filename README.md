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
This was originally developed for embedded x86 systems, but it's fairly modular.
With a little effort, it can be easily ported to other platforms. I recommend
starting with the mock architecture, and modifying `gdbstub_sys.h` and
`gdbstub_sys.c` to fit your platform's needs accordingly. If you port it to
another platform, I welcome a PR for it!

Building
--------
By default, running `make` produces a `gdbstub-mock` program. This is simply a
stub for a mock architecture (just a handful of registers and some memory)
running inside a normal program that communicates over stdio.

A stub intended for bare metal x86 machines can be built with `make ARCH=x86`.
This produces an ELF binary `gdbstub-x86.elf` with an entry point `dbg_start`
that will hook the current IDT (to support debug interrupts) and break.

Additionally, a simple flat binary `gdbstub-x86.bin` is created from the ELF
binary. The intent for this flat binary is to be trivially loaded and jumped
to.

x86 Demo
--------
In `demo/demo.c` there is a simple function that's used for demonstration and
testing. To use it, first build the stub with `make ARCH=x86 INCLUDE_DEMO=1`.
Then, to test the GDB stub out, launch an instance of the full-system emulator
[QEMU](https://www.qemu.org/) as follows:

	qemu-system-i386 -serial tcp:127.0.0.1:1234,server -display none -kernel gdbstub-x86.elf

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
