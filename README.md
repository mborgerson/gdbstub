gdbstub
=======
This is a simple, single-file[^1] GDB stub library that can be easily dropped in to
your project to allow you to debug a target platform using GDB (or another
application which supports remote GDB targets). It has no library dependencies
(such as libc) and requires just standard tools to build.

[^1]: The main protocol bits and most architecture support is in a single
      file: gdbstub.h. Some platform files might also be required depending on
      your use case.

Protocol
--------
Communication between the stub and the debugger takes place via the [GDB
Remote Serial Protocol](https://sourceware.org/gdb/onlinedocs/gdb/Remote-Protocol.html).

Usage
-----
In some `.c` file in your project:

```c
/* Define to enable bare metal x86 support, or define your own architecture */
#define GDBSTUB_ARCH_X86

/* Enable function definitions */
#define GDBSTUB_IMPLEMENTATION

#include "gdbstub.h"

void _start(void) {
	dbg_sys_init(); /* Setup, wait for debugger on serial port  */
}
```

See `gdbstub.c` for example usage.

Architecture Support
--------------------
* `GDBSTUB_ARCH_MOCK`: A mock architecture for testing
* `GDBSTUB_ARCH_X86`: Bare-metal x86 (32-bit). You'll also need interrupt handlers (so not .

Porting
-------
This was originally developed for embedded x86 systems, but it's fairly modular.
With a little effort, it can be easily ported to other platforms.

I recommend copying the mock architecture implementation and adjusting it to fit
your platform's needs accordingly.

PR's for other platforms are welcome!

Building
--------
By default, running `make` produces a `gdbstub` program. This is simply a stub
for a mock architecture (just a handful of registers and some memory) running
inside a normal program that communicates over stdio.

A stub intended for bare metal x86 machines can be built with `make ARCH=x86`.
This produces an ELF binary `gdbstub.elf` that will hook the current IDT
(to support debug interrupts) and break.

Additionally, a simple flat binary `gdbstub.bin` is created from the ELF binary.
The intent for this flat binary is to be easily loaded into memory and jumped
to.

x86 Demo
--------
In `gdbstub.c` there is a simple function that's used for demonstration and
testing. To use it, build the stub with:

	$ make ARCH=x86 INCLUDE_DEMO=1

Then, to test the GDB stub out, launch an instance of the full-system emulator
[QEMU](https://www.qemu.org/) as follows:

	qemu-system-i386 -serial tcp:127.0.0.1:1234,server -display none -kernel gdbstub.elf

This will launch QEMU, create a virtual machine with a virtual serial port that
can be connected to through local TCP port 1234, then load and run the stub
executable inside the virtual machine.

You can then launch your local GDB client to get your GDB client to connect to
the virtual serial port and begin debugging the demo application:

	$ gdb
	(gdb) symbol-file gdbstub.elf
	(gdb) target remote 127.0.0.1:1234
	(gdb) b simple_loop
	(gdb) layout split
	(gdb) c

For example, step a couple of times and print out the value of `x`:

	(gdb) s 3
	(gdb) p/x x
	$1 = 0xdeadbeef

License
-------
This software is published under the terms of the MIT License. See `LICENSE.txt`
for full license.

Matt Borgerson, 2016-2022
