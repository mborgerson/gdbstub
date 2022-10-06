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

CFLAGS       = -Werror -ansi -g -Iarch_$(ARCH) -I$(PWD)
NASM         = nasm
NASM_FLAGS   = -felf
OBJCOPY      = objcopy
OBJCOPYFLAGS = --output-target=binary
BASE_ADDRESS = 0x500000
TARGET       = gdbstub-$(ARCH).bin
OBJECTS      = gdbstub.o \
               arch_$(ARCH)/gdbstub_sys.o

INCLUDE_DEMO ?= 0

ifeq ($(ARCH),mock)
CFLAGS += -DDEFINE_MAIN -DUSE_STDIO
TARGET = gdbstub-mock
INCLUDE_DEMO = 0
else ifeq ($(ARCH),x86)
CFLAGS  += -Os -m32 -ffunction-sections -fno-stack-protector
LDFLAGS += -m elf_i386 --gc-sections
OBJECTS += arch_$(ARCH)/gdbstub_int.o
else
$(error Please specify a supported architecture)
endif

ifeq ($(INCLUDE_DEMO),1)
	OBJECTS += demo/demo.o
endif

all: $(TARGET)

gdbstub-mock: $(OBJECTS)
	$(CC) -o $@ $^

%.bin: %.elf
	$(OBJCOPY) $(OBJCOPYFLAGS) $^ $@

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
	$(NASM) -o $@ $(NASM_FLAGS) $<

.PHONY: clean
clean:
	rm -f \
		$(TARGET) \
		$(TARGET:.bin=.elf) \
		$(OBJECTS) \
		gdbstub.ld
