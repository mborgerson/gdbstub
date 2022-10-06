/*
 * Copyright (c) 2022 Matt Borgerson
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

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "gdbstub.h"

static char dbg_mem[256];

struct dbg_buffer {
	char   *buf;
	size_t  pos_write;
	size_t  pos_read;
	size_t  size;
} dbg_input, dbg_output;

void dbg_buf_write(struct dbg_buffer *buf, int ch)
{
	if (buf->buf == NULL) {
		buf->pos_write = 0;
		buf->pos_read = 0;
		buf->size = 512;
		buf->buf = malloc(buf->size);
	} else if (buf->pos_write >= buf->size) {
		buf->size *= 2;
		buf->buf = realloc(buf->buf, buf->size);
	}

	assert(buf->buf);
	buf->buf[buf->pos_write++] = ch;
}

int dbg_buf_read(struct dbg_buffer *buf)
{
	if (buf->buf && buf->pos_read < buf->pos_write) {
		return buf->buf[buf->pos_read++];
	}
	return EOF;
}

/*****************************************************************************
 * Debugging System Functions
 ****************************************************************************/

/*
 * Write one character to the debugging stream.
 */
int dbg_sys_putchar(struct dbg_state *state, int ch)
{
#ifdef USE_STDIO
	putchar(ch);
#else
	dbg_buf_write(&dbg_output, ch);
#endif
	return 0;
}

/*
 * Read one character from the debugging stream.
 */
int dbg_sys_getc(struct dbg_state *state)
{
#ifdef USE_STDIO
	int ch = getchar();
	return ch == EOF ? DBG_EOF : ch;
#else
	return dbg_buf_read(&dbg_input);
#endif
}

/*
 * Read one byte from memory.
 */
int dbg_sys_mem_readb(struct dbg_state *state, address addr, char *val)
{
	if (addr >= sizeof(dbg_mem)) {
		return 1;
	}

	*val = dbg_mem[addr];
	return 0;
}

/*
 * Write one byte to memory.
 */
int dbg_sys_mem_writeb(struct dbg_state *state, address addr, char val)
{
	if (addr >= sizeof(dbg_mem)) {
		return 1;
	}

	dbg_mem[addr] = val;
	return 0;
}

/*
 * Continue program execution.
 */
int dbg_sys_continue(struct dbg_state *state)
{
	return 0;
}

/*
 * Single step the next instruction.
 */
int dbg_sys_step(struct dbg_state *state)
{
	return 0;
}

#ifdef DEFINE_MAIN
int main(int argc, char const *argv[])
{
	struct dbg_state state;
	state.signum = 5;
	memset(&state.registers, 0, sizeof(state.registers));
	while (!feof(stdin)) {
		dbg_main(&state);
	}
	return 0;
}
#endif
