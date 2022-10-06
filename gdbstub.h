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

#ifndef _GDBSTUB_H_
#define _GDBSTUB_H_

/* Enable debug statements (printf) */
#ifndef DEBUG
#define DEBUG 0
#endif

#if DEBUG
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#endif

/* Include platform specific definitions */
#include "gdbstub_sys.h"

/*****************************************************************************
 * Macros
 ****************************************************************************/

#if DEBUG
#define DBG_PRINT(...) fprintf(stderr, __VA_ARGS__)
#else
#define DBG_PRINT(...)
#endif

#ifndef DBG_EOF
#define DBG_EOF (-1)
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

#ifndef DBG_ASSERT
#if DEBUG
#define DBG_ASSERT(x) { \
	if (!(x)) { \
		fprintf(stderr, "ASSERTION FAILED\n"); \
		fprintf(stderr, "  Assertion: %s\n", #x); \
		fprintf(stderr, "  Location:  %s @ %s:%d\n", __func__, \
		                                             __FILE__, __LINE__); \
		exit(1); \
	} \
}
#else
#define DBG_ASSERT(x) \
	do {} while (0)
#endif
#endif

/*****************************************************************************
 * Prototypes
 ****************************************************************************/

int dbg_main(struct dbg_state *state);

/* System functions, supported by all stubs */
int dbg_sys_getc(struct dbg_state *state);
int dbg_sys_putchar(struct dbg_state *state, int ch);
int dbg_sys_mem_readb(struct dbg_state *state, address addr, char *val);
int dbg_sys_mem_writeb(struct dbg_state *state, address addr, char val);
int dbg_sys_continue(struct dbg_state *state);
int dbg_sys_step(struct dbg_state *state);

#endif
