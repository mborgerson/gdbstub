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

#ifdef GDBSTUB_ARCH_MOCK
#define USE_STDIO
#endif

#define GDBSTUB_IMPLEMENTATION
#include "gdbstub.h"

#ifdef GDBSTUB_ARCH_MOCK

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

#else

void simple_loop(void)
{
	volatile int x;
	int i;

	x = 0xdeadbeef;
	for (i = 0; 1; i++) {
		x ^= (1 << (i % 32));
	}
}

/* This will be called at startup. */
__attribute__((section(".text._start")))
void _start(void)
{
	/* Enable debugging hooks and break */
	dbg_sys_init();

	/* Example code to debug through... */
	simple_loop();
}

#endif
