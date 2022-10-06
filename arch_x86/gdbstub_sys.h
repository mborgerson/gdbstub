/*
 * Copyright (c) 2016-2022 Matt Borgerson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _GDBSTUB_SYS_H_
#define _GDBSTUB_SYS_H_

/* Define the size_t type */
#define DBG_DEFINE_SIZET 1

/* Define required standard integer types (e.g. uint16_t) */
#define DBG_DEFINE_STDINT 1

/*****************************************************************************
 * Types
 ****************************************************************************/

#if DBG_DEFINE_STDINT
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long  uint32_t;
#endif

#if DBG_DEFINE_SIZET
typedef unsigned int size_t;
#endif

typedef unsigned int address;
typedef unsigned int reg;

#pragma pack(1)
struct dbg_interrupt_state {
	uint32_t ss;
	uint32_t gs;
	uint32_t fs;
	uint32_t es;
	uint32_t ds;
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t esp;
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
	uint32_t vector;
	uint32_t error_code;
	uint32_t eip;
	uint32_t cs;
	uint32_t eflags;
};
#pragma pack()

#pragma pack(1)
struct dbg_idtr
{
	uint16_t len;
	uint32_t offset;
};
#pragma pack()

#pragma pack(1)
struct dbg_idt_gate
{
	uint16_t offset_low;
	uint16_t segment;
	uint16_t flags;
	uint16_t offset_high;
};
#pragma pack()

enum DBG_REGISTER {
	DBG_CPU_I386_REG_EAX  = 0,
	DBG_CPU_I386_REG_ECX  = 1,
	DBG_CPU_I386_REG_EDX  = 2,
	DBG_CPU_I386_REG_EBX  = 3,
	DBG_CPU_I386_REG_ESP  = 4,
	DBG_CPU_I386_REG_EBP  = 5,
	DBG_CPU_I386_REG_ESI  = 6,
	DBG_CPU_I386_REG_EDI  = 7,
	DBG_CPU_I386_REG_PC   = 8,
	DBG_CPU_I386_REG_PS   = 9,
	DBG_CPU_I386_REG_CS   = 10,
	DBG_CPU_I386_REG_SS   = 11,
	DBG_CPU_I386_REG_DS   = 12,
	DBG_CPU_I386_REG_ES   = 13,
	DBG_CPU_I386_REG_FS   = 14,
	DBG_CPU_I386_REG_GS   = 15,
	DBG_CPU_NUM_REGISTERS = 16
};

struct dbg_state {
	int signum;
	reg registers[DBG_CPU_NUM_REGISTERS];
};

/*****************************************************************************
 * Const Data
 ****************************************************************************/

extern void const * const dbg_int_handlers[];

/*****************************************************************************
 * Prototypes
 ****************************************************************************/

void dbg_hook_idt(uint8_t vector, const void *function);
void dbg_init_gates(void);
void dbg_init_idt(void);
void dbg_load_idt(struct dbg_idtr *idtr);
void dbg_store_idt(struct dbg_idtr *idtr);
uint32_t dbg_get_cs(void);
void dbg_int_handler(struct dbg_interrupt_state *istate);
void dbg_interrupt(struct dbg_interrupt_state *istate);
void dbg_start(void);
void dbg_io_write_8(uint16_t port, uint8_t val);
uint8_t dbg_io_read_8(uint16_t port);
void *dbg_sys_memset(void *ptr, int data, size_t len);

#endif
