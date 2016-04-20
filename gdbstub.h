/*
 * Copyright (C) 2016  Matt Borgerson
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _GDBSTUB_H_
#define _GDBSTUB_H_

/* Enable debug statements (printf) */
#define DEBUG 0

/* Include platform specific definitions */
#include "gdbstub_sys.h"

/*****************************************************************************
 * Macros
 ****************************************************************************/

#if DEBUG
#define DEBUG_PRINT(...) fprintf(stderr, __VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#endif

#ifndef EOF
#define EOF (-1)
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

#ifndef ASSERT
#if DEBUG
#define ASSERT(x) { \
	if (!(x)) { \
		fprintf(stderr, "ASSERTION FAILED\n"); \
		fprintf(stderr, "  Assertion: %s\n", #x); \
		fprintf(stderr, "  Location:  %s @ %s:%d\n", __func__, \
		                                             __FILE__, __LINE__); \
		exit(1); \
	} \
}
#else
#define ASSERT(x) { \
 	while(1); \
}
#endif
#endif

/*****************************************************************************
 * Prototypes
 ****************************************************************************/

int dbg_main(struct dbg_state *state);

/* System functions, supported by all stubs */
int dbg_sys_getc(void);
int dbg_sys_putchar(int ch);
int dbg_sys_mem_readb(address addr, char *val);
int dbg_sys_mem_writeb(address addr, char val);
int dbg_sys_continue();
int dbg_sys_step();

#endif