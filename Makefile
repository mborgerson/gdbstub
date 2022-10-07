#
# Copyright (c) 2016-2022 Matt Borgerson
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

ARCH ?= mock

CFLAGS       = -Werror -ansi -g
OBJCOPY      = objcopy
BASE_ADDRESS = 0x500000
TARGET       = gdbstub.bin
OBJECTS      = gdbstub.o
INCLUDE_DEMO ?= 0

ifeq ($(ARCH),mock)
CFLAGS += -DGDBSTUB_ARCH_MOCK
TARGET = gdbstub
INCLUDE_DEMO = 0
else
GENERATED += gdbstub.elf gdbstub.ld
ifeq ($(ARCH),x86)
CFLAGS  += -Os -m32 -fno-stack-protector -DGDBSTUB_ARCH_X86 \
           -DINCLUDE_DEMO=$(INCLUDE_DEMO)
LDFLAGS += -m elf_i386
OBJECTS += gdbstub_x86_int.o
else
$(error Please specify a supported architecture)
endif
endif

GENERATED += $(TARGET) $(OBJECTS)

all: $(TARGET)

gdbstub: $(OBJECTS)
	$(CC) -o $@ $^

%.bin: %.elf
	$(OBJCOPY) --output-target=binary $^ $@

.PRECIOUS: %.elf
%.elf: $(OBJECTS) gdbstub.ld
	$(LD) --script=gdbstub.ld $(LDFLAGS) -o $@ $(OBJECTS)

gdbstub.ld: gdbstub.ld.in Makefile
	$(CC) -o $@ -x c -P -E \
		-DBASE_ADDRESS=$(BASE_ADDRESS) \
		-DINCLUDE_DEMO=$(INCLUDE_DEMO) \
		$<

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

%.o: %.nasm
	nasm -o $@ -felf $<

.PHONY: clean
clean:
	rm -f $(GENERATED)
