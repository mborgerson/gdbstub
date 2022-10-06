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

ARCH := arch_x86

# Include a simple demo that can be debugged
INCLUDE_DEMO = 1

CC           = gcc
CFLAGS       = -Werror -ansi -Os -g -ffunction-sections -fno-stack-protector -I$(ARCH) -I$(PWD)
LD           = ld
LDFLAGS      = --script=gdbstub.ld --gc-sections
NASM         = nasm
NASM_FLAGS   = -felf
OBJCOPY      = objcopy
OBJCOPYFLAGS = --output-target=binary
TARGET       = gdbstub.bin
BASE_ADDRESS = 0x500000
OBJECTS      = gdbstub.o \
               $(ARCH)/gdbstub_sys.o

ifeq ($(ARCH),arch_x86)
CFLAGS  += -m32
LDFLAGS += -m elf_i386
OBJECTS += $(ARCH)/gdbstub_int.o
else
$(error Please specify a supported architecture)
endif

ifeq ($(INCLUDE_DEMO),1)
OBJECTS += demo/demo.o
endif

all: $(TARGET)
.PRECIOUS: %.elf

%.bin: %.elf
	$(OBJCOPY) $(OBJCOPYFLAGS) $^ $@

%.elf: $(OBJECTS) gdbstub.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS)

gdbstub.ld: gdbstub.ld.in Makefile
	$(CC) -o $@ -x c -P -E \
		-DBASE_ADDRESS=$(BASE_ADDRESS) \
		-DINCLUDE_DEMO=$(INCLUDE_DEMO) \
		$<

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

%.o: %.nasm
	$(NASM) -o $@ $(NASM_FLAGS) $<

.PHONY: clean
clean:
	rm -f \
		$(TARGET) \
		$(TARGET:.bin=.elf) \
		$(OBJECTS) \
		gdbstub.ld
