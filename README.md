gdbstub
=======
This is a simple gdb stub that can be easily dropped in to your project. It has
no library dependencies (such as libc) and requires just standard tools to
build.

Protocol
--------
Communication between the stub and the debugger takes place via the [gdb
Remote Serial Protocol](https://sourceware.org/gdb/onlinedocs/gdb/Remote-Protocol.html).

Porting
-------
This was developed for x86 systems, but it's fairly modular. With a little
effort, it can be easily ported to other platforms. You will need to modify
`gdbstub_sys.h` and `gdbstub_sys.c` to fit your platform's needs accordingly.

Building
--------
Running `make` produces a simple binary that will hook the current IDT and
break.

License
-------
This software is published under the terms of the MIT License. See `LICENSE.txt`
for full license.

Matt Borgerson, 2016-2019
