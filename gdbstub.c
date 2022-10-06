/*
 * Copyright (c) 2016-2019 Matt Borgerson
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

#include "gdbstub.h"

/*****************************************************************************
 * Types
 ****************************************************************************/

typedef int (*dbg_enc_func)(char *buf, size_t buf_len, const char *data,
                            size_t data_len);
typedef int (*dbg_dec_func)(const char *buf, size_t buf_len, char *data,
                            size_t data_len);

/*****************************************************************************
 * Const Data
 ****************************************************************************/

const char digits[] = "0123456789abcdef";

/*****************************************************************************
 * Prototypes
 ****************************************************************************/

/* Communication functions */
int dbg_write(struct dbg_state *state, const char *buf, size_t len);
int dbg_read(struct dbg_state *state, char *buf, size_t buf_len, size_t len);

/* String processing helper functions */
int dbg_strlen(const char *ch);
int dbg_is_printable_char(char ch);
char dbg_get_digit(int val);
int dbg_get_val(char digit, int base);
int dbg_strtol(const char *str, size_t len, int base, const char **endptr);

/* Packet functions */
int dbg_send_packet(struct dbg_state *state, const char *pkt, size_t pkt_len);
int dbg_recv_packet(struct dbg_state *state, char *pkt_buf, size_t pkt_buf_len,
                    size_t *pkt_len);
int dbg_checksum(const char *buf, size_t len);
int dbg_recv_ack(struct dbg_state *state);

/* Data encoding/decoding */
int dbg_enc_hex(char *buf, size_t buf_len, const char *data, size_t data_len);
int dbg_dec_hex(const char *buf, size_t buf_len, char *data, size_t data_len);
int dbg_enc_bin(char *buf, size_t buf_len, const char *data, size_t data_len);
int dbg_dec_bin(const char *buf, size_t buf_len, char *data, size_t data_len);

/* Packet creation helpers */
int dbg_send_ok_packet(struct dbg_state *state, char *buf, size_t buf_len);
int dbg_send_conmsg_packet(struct dbg_state *state, char *buf, size_t buf_len,
                           const char *msg);
int dbg_send_signal_packet(struct dbg_state *state, char *buf, size_t buf_len,
                           char signal);
int dbg_send_error_packet(struct dbg_state *state, char *buf, size_t buf_len,
                          char error);

/* Command functions */
int dbg_mem_read(struct dbg_state *state, char *buf, size_t buf_len,
                 address addr, size_t len, dbg_enc_func enc);
int dbg_mem_write(struct dbg_state *state, const char *buf, size_t buf_len,
                  address addr, size_t len, dbg_dec_func dec);
int dbg_continue(struct dbg_state *state);
int dbg_step(struct dbg_state *state);

/*****************************************************************************
 * String Processing Helper Functions
 ****************************************************************************/

/*
 * Get null-terminated string length.
 */
int dbg_strlen(const char *ch)
{
	int len;

	len = 0;
	while (*ch++) {
		len += 1;
	}

	return len;
}

/*
 * Get integer value for a string representation.
 *
 * If the string starts with + or -, it will be signed accordingly.
 *
 * If base == 0, the base will be determined:
 *   base 16 if the string starts with 0x or 0X,
 *   base 10 otherwise
 *
 * If endptr is specified, it will point to the last non-digit in the
 * string. If there are no digits in the string, it will be set to NULL.
 */
int dbg_strtol(const char *str, size_t len, int base, const char **endptr)
{
	size_t pos;
	int sign, tmp, value, valid;

	value = 0;
	pos   = 0;
	sign  = 1;
	valid = 0;

	if (endptr) {
		*endptr = NULL;
	}

	if (len < 1) {
		return 0;
	}

	/* Detect negative numbers */
	if (str[pos] == '-') {
		sign = -1;
		pos += 1;
	} else if (str[pos] == '+') {
		sign = 1;
		pos += 1;
	}

	/* Detect '0x' hex prefix */
	if ((pos + 2 < len) && (str[pos] == '0') &&
		((str[pos+1] == 'x') || (str[pos+1] == 'X'))) {
		base = 16;
		pos += 2;
	}

	if (base == 0) {
		base = 10;
	}

	for (; (pos < len) && (str[pos] != '\x00'); pos++) {
		tmp = dbg_get_val(str[pos], base);
		if (tmp == EOF) {
			break;
		}

		value = value*base + tmp;
		valid = 1; /* At least one digit is valid */
	}

	if (!valid) {
		return 0;
	}

	if (endptr) {
		*endptr = str+pos;
	}

	value *= sign;

	return value;
}

/*
 * Get the corresponding ASCII hex digit character for a value.
 */
char dbg_get_digit(int val)
{
	if ((val >= 0) && (val <= 0xf)) {
		return digits[val];
	} else {
		return EOF;
	}
}

/*
 * Get the corresponding value for a ASCII digit character.
 *
 * Supports bases 2-16.
 */
int dbg_get_val(char digit, int base)
{
	int value;

	if ((digit >= '0') && (digit <= '9')) {
		value = digit-'0';
	} else if ((digit >= 'a') && (digit <= 'f')) {
		value = digit-'a'+0xa;
	} else if ((digit >= 'A') && (digit <= 'F')) {
		value = digit-'A'+0xa;
	} else {
		return EOF;
	}

	return (value < base) ? value : EOF;
}

/*
 * Determine if this is a printable ASCII character.
 */
int dbg_is_printable_char(char ch)
{
	return (ch >= 0x20 && ch <= 0x7e);
}

/*****************************************************************************
 * Packet Functions
 ****************************************************************************/

/*
 * Receive a packet acknowledgment
 *
 * Returns:
 *    0   if an ACK (+) was received
 *    1   if a NACK (-) was received
 *    EOF otherwise
 */
int dbg_recv_ack(struct dbg_state *state)
{
	int response;

	/* Wait for packet ack */
	switch (response = dbg_sys_getc(state)) {
	case '+':
		/* Packet acknowledged */
		return 0;
	case '-':
		/* Packet negative acknowledged */
		return 1;
	default:
		/* Bad response! */
		DEBUG_PRINT("received bad packet response: 0x%2x\n", response);
		return EOF;
	}
}

/*
 * Calculate 8-bit checksum of a buffer.
 *
 * Returns:
 *    8-bit checksum.
 */
int dbg_checksum(const char *buf, size_t len)
{
	unsigned char csum;

	csum = 0;

	while (len--) {
		csum += *buf++;
	}

	return csum;
}

/*
 * Transmits a packet of data.
 * Packets are of the form: $<packet-data>#<checksum>
 *
 * Returns:
 *    0   if the packet was transmitted and acknowledged
 *    1   if the packet was transmitted but not acknowledged
 *    EOF otherwise
 */
int dbg_send_packet(struct dbg_state *state, const char *pkt_data,
                    size_t pkt_len)
{
	char buf[3];
	char csum;

	/* Send packet start */
	if (dbg_sys_putchar(state, '$') == EOF) {
		return EOF;
	}

#if DEBUG
	{
		size_t p;
		DEBUG_PRINT("-> ");
		for (p = 0; p < pkt_len; p++) {
			if (dbg_is_printable_char(pkt_data[p])) {
				DEBUG_PRINT("%c", pkt_data[p]);
			} else {
				DEBUG_PRINT("\\x%02x", pkt_data[p]&0xff);
			}
		}
		DEBUG_PRINT("\n");
	}
#endif

	/* Send packet data */
	if (dbg_write(state, pkt_data, pkt_len) == EOF) {
		return EOF;
	}

	/* Send the checksum */
	buf[0] = '#';
	csum = dbg_checksum(pkt_data, pkt_len);
	if ((dbg_enc_hex(buf+1, sizeof(buf)-1, &csum, 1) == EOF) ||
		(dbg_write(state, buf, sizeof(buf)) == EOF)) {
		return EOF;
	}

	return dbg_recv_ack(state);
}

/*
 * Receives a packet of data, assuming a 7-bit clean connection.
 *
 * Returns:
 *    0   if the packet was received
 *    EOF otherwise
 */
int dbg_recv_packet(struct dbg_state *state, char *pkt_buf, size_t pkt_buf_len,
                    size_t *pkt_len)
{
	int data;
	char expected_csum, actual_csum;
	char buf[2];

	/* Wait for packet start */
	actual_csum = 0;

	while (1) {
		data = dbg_sys_getc(state);
		if (data == EOF) {
			return EOF;
		} else if (data == '$') {
			/* Detected start of packet. */
			break;
		}
	}

	/* Read until checksum */
	*pkt_len = 0;
	while (1) {
		data = dbg_sys_getc(state);

		if (data == EOF) {
			/* Error receiving character */
			return EOF;
		} else if (data == '#') {
			/* End of packet */
			break;
		} else {
			/* Check for space */
			if (*pkt_len >= pkt_buf_len) {
				DEBUG_PRINT("packet buffer overflow\n");
				return EOF;
			}

			/* Store character and update checksum */
			pkt_buf[(*pkt_len)++] = (char) data;
		}
	}

#if DEBUG
	{
		size_t p;
		DEBUG_PRINT("<- ");
		for (p = 0; p < *pkt_len; p++) {
			if (dbg_is_printable_char(pkt_buf[p])) {
				DEBUG_PRINT("%c", pkt_buf[p]);
			} else {
				DEBUG_PRINT("\\x%02x", pkt_buf[p] & 0xff);
			}
		}
		DEBUG_PRINT("\n");
	}
#endif

	/* Receive the checksum */
	if ((dbg_read(state, buf, sizeof(buf), 2) == EOF) ||
		(dbg_dec_hex(buf, 2, &expected_csum, 1) == EOF)) {
		return EOF;
	}

	/* Verify checksum */
	actual_csum = dbg_checksum(pkt_buf, *pkt_len);
	if (actual_csum != expected_csum) {
		/* Send packet nack */
		DEBUG_PRINT("received packet with bad checksum\n");
		dbg_sys_putchar(state, '-');
		return EOF;
	}

	/* Send packet ack */
	dbg_sys_putchar(state, '+');
	return 0;
}

/*****************************************************************************
 * Data Encoding/Decoding
 ****************************************************************************/

/*
 * Encode data to its hex-value representation in a buffer.
 *
 * Returns:
 *    0+  number of bytes written to buf
 *    EOF if the buffer is too small
 */
int dbg_enc_hex(char *buf, size_t buf_len, const char *data, size_t data_len)
{
	size_t pos;

	if (buf_len < data_len*2) {
		/* Buffer too small */
		return EOF;
	}

	for (pos = 0; pos < data_len; pos++) {
		*buf++ = dbg_get_digit((data[pos] >> 4) & 0xf);
		*buf++ = dbg_get_digit((data[pos]     ) & 0xf);
	}

	return data_len*2;
}

/*
 * Decode data from its hex-value representation to a buffer.
 *
 * Returns:
 *    0   if successful
 *    EOF if the buffer is too small
 */
int dbg_dec_hex(const char *buf, size_t buf_len, char *data, size_t data_len)
{
	size_t pos;
	int tmp;

	if (buf_len != data_len*2) {
		/* Buffer too small */
		return EOF;
	}

	for (pos = 0; pos < data_len; pos++) {
		/* Decode high nibble */
		tmp = dbg_get_val(*buf++, 16);
		if (tmp == EOF) {
			/* Buffer contained junk. */
			ASSERT(0);
			return EOF;
		}

		data[pos] = tmp << 4;

		/* Decode low nibble */
		tmp = dbg_get_val(*buf++, 16);
		if (tmp == EOF) {
			/* Buffer contained junk. */
			ASSERT(0);
			return EOF;
		}
		data[pos] |= tmp;
	}

	return 0;
}

/*
 * Encode data to its binary representation in a buffer.
 *
 * Returns:
 *    0+  number of bytes written to buf
 *    EOF if the buffer is too small
 */
int dbg_enc_bin(char *buf, size_t buf_len, const char *data, size_t data_len)
{
	size_t buf_pos, data_pos;

	for (buf_pos = 0, data_pos = 0; data_pos < data_len; data_pos++) {
		if (data[data_pos] == '$' ||
			data[data_pos] == '#' ||
			data[data_pos] == '}' ||
			data[data_pos] == '*') {
			if (buf_pos+1 >= buf_len) {
				ASSERT(0);
				return EOF;
			}
			buf[buf_pos++] = '}';
			buf[buf_pos++] = data[data_pos] ^ 0x20;
		} else {
			if (buf_pos >= buf_len) {
				ASSERT(0);
				return EOF;
			}
			buf[buf_pos++] = data[data_pos];
		}
	}

	return buf_pos;
}

/*
 * Decode data from its bin-value representation to a buffer.
 *
 * Returns:
 *    0+  if successful, number of bytes decoded
 *    EOF if the buffer is too small
 */
int dbg_dec_bin(const char *buf, size_t buf_len, char *data, size_t data_len)
{
	size_t buf_pos, data_pos;

	for (buf_pos = 0, data_pos = 0; buf_pos < buf_len; buf_pos++) {
		if (data_pos >= data_len) {
			/* Output buffer overflow */
			ASSERT(0);
			return EOF;
		}
		if (buf[buf_pos] == '}') {
			/* The next byte is escaped! */
			if (buf_pos+1 >= buf_len) {
				/* There's an escape character, but no escaped character
				 * following the escape character. */
				ASSERT(0);
				return EOF;
			}
			buf_pos += 1;
			data[data_pos++] = buf[buf_pos] ^ 0x20;
		} else {
			data[data_pos++] = buf[buf_pos];
		}
	}

	return data_pos;
}

/*****************************************************************************
 * Command Functions
 ****************************************************************************/

/*
 * Read from memory and encode into buf.
 *
 * Returns:
 *    0+  number of bytes written to buf
 *    EOF if the buffer is too small
 */
int dbg_mem_read(struct dbg_state *state, char *buf, size_t buf_len,
                 address addr, size_t len, dbg_enc_func enc)
{
	char data[64];
	size_t pos;

	if (len > sizeof(data)) {
		return EOF;
	}

	/* Read from system memory */
	for (pos = 0; pos < len; pos++) {
		if (dbg_sys_mem_readb(state, addr+pos, &data[pos])) {
			/* Failed to read */
			return EOF;
		}
	}

	/* Encode data */
	return enc(buf, buf_len, data, len);
}

/*
 * Write to memory from encoded buf.
 */
int dbg_mem_write(struct dbg_state *state, const char *buf, size_t buf_len,
                  address addr, size_t len, dbg_dec_func dec)
{
	char data[64];
	size_t pos;

	if (len > sizeof(data)) {
		return EOF;
	}

	/* Decode data */
	if (dec(buf, buf_len, data, len) == EOF) {
		return EOF;
	}

	/* Write to system memory */
	for (pos = 0; pos < len; pos++) {
		if (dbg_sys_mem_writeb(state, addr+pos, data[pos])) {
			/* Failed to write */
			return EOF;
		}
	}

	return 0;
}

/*
 * Continue program execution at PC.
 */
int dbg_continue(struct dbg_state *state)
{
	dbg_sys_continue(state);
	return 0;
}

/*
 * Step one instruction.
 */
int dbg_step(struct dbg_state *state)
{
	dbg_sys_step(state);
	return 0;
}

/*****************************************************************************
 * Packet Creation Helpers
 ****************************************************************************/

/*
 * Send OK packet
 */
int dbg_send_ok_packet(struct dbg_state *state, char *buf, size_t buf_len)
{
	return dbg_send_packet(state, "OK", 2);
}

/*
 * Send a message to the debugging console (via O XX... packet)
 */
int dbg_send_conmsg_packet(struct dbg_state *state, char *buf, size_t buf_len,
                           const char *msg)
{
	size_t size;
	int status;

	if (buf_len < 2) {
		/* Buffer too small */
		return EOF;
	}

	buf[0] = 'O';
	status = dbg_enc_hex(&buf[1], buf_len-1, msg, dbg_strlen(msg));
	if (status == EOF) {
		return EOF;
	}
	size = 1 + status;
	return dbg_send_packet(state, buf, size);
}

/*
 * Send a signal packet (S AA).
 */
int dbg_send_signal_packet(struct dbg_state *state, char *buf, size_t buf_len,
                           char signal)
{
	size_t size;
	int status;

	if (buf_len < 4) {
		/* Buffer too small */
		return EOF;
	}

	buf[0] = 'S';
	status = dbg_enc_hex(&buf[1], buf_len-1, &signal, 1);
	if (status == EOF) {
		return EOF;
	}
	size = 1 + status;
	return dbg_send_packet(state, buf, size);
}

/*
 * Send a error packet (E AA).
 */
int dbg_send_error_packet(struct dbg_state *state, char *buf, size_t buf_len,
                          char error)
{
	size_t size;
	int status;

	if (buf_len < 4) {
		/* Buffer too small */
		return EOF;
	}

	buf[0] = 'E';
	status = dbg_enc_hex(&buf[1], buf_len-1, &error, 1);
	if (status == EOF) {
		return EOF;
	}
	size = 1 + status;
	return dbg_send_packet(state, buf, size);
}

/*****************************************************************************
 * Communication Functions
 ****************************************************************************/

/*
 * Write a sequence of bytes.
 *
 * Returns:
 *    0   if successful
 *    EOF if failed to write all bytes
 */
int dbg_write(struct dbg_state *state, const char *buf, size_t len)
{
	while (len--) {
		if (dbg_sys_putchar(state, *buf++) == EOF) {
			return EOF;
		}
	}

	return 0;
}

/*
 * Read a sequence of bytes.
 *
 * Returns:
 *    0   if successfully read len bytes
 *    EOF if failed to read all bytes
 */
int dbg_read(struct dbg_state *state, char *buf, size_t buf_len, size_t len)
{
	char c;

	if (buf_len < len) {
		/* Buffer too small */
		return EOF;
	}

	while (len--) {
		if ((c = dbg_sys_getc(state)) == EOF) {
			return EOF;
		}
		*buf++ = c;
	}

	return 0;
}

/*****************************************************************************
 * Main Loop
 ****************************************************************************/

/*
 * Main debug loop. Handles commands.
 */
int dbg_main(struct dbg_state *state)
{
	address     addr;
	char        pkt_buf[256];
	int         status;
	size_t      length;
	size_t      pkt_len;
	const char *ptr_next;

	dbg_send_signal_packet(state, pkt_buf, sizeof(pkt_buf), state->signum);

	while (1) {
		/* Receive the next packet */
		status = dbg_recv_packet(state, pkt_buf, sizeof(pkt_buf), &pkt_len);
		if (status == EOF) {
			break;
		}

		if (pkt_len == 0) {
			/* Received empty packet.. */
			continue;
		}

		ptr_next = pkt_buf;

		/*
		 * Handle one letter commands
		 */
		switch (pkt_buf[0]) {

		/* Calculate remaining space in packet from ptr_next position. */
		#define token_remaining_buf (pkt_len-(ptr_next-pkt_buf))

		/* Expecting a seperator. If not present, go to error */
		#define token_expect_seperator(c) \
			{ \
				if (!ptr_next || *ptr_next != c) { \
					goto error; \
				} else { \
					ptr_next += 1; \
				} \
			}

		/* Expecting an integer argument. If not present, go to error */
		#define token_expect_integer_arg(arg) \
			{ \
				arg = dbg_strtol(ptr_next, token_remaining_buf, \
				                 16, &ptr_next); \
				if (!ptr_next) { \
					goto error; \
				} \
			}

		/*
		 * Read Registers
		 * Command Format: g
		 */
		case 'g':
			/* Encode registers */
			status = dbg_enc_hex(pkt_buf, sizeof(pkt_buf),
			                     (char *)&(state->registers),
			                     sizeof(state->registers));
			if (status == EOF) {
				goto error;
			}
			pkt_len = status;
			dbg_send_packet(state, pkt_buf, pkt_len);
			break;
		
		/*
		 * Write Registers
		 * Command Format: G XX...
		 */
		case 'G':
			status = dbg_dec_hex(pkt_buf+1, pkt_len-1,
			                     (char *)&(state->registers),
			                     sizeof(state->registers));
			if (status == EOF) {
				goto error;
			}
			dbg_send_ok_packet(state, pkt_buf, sizeof(pkt_buf));
			break;

		/*
		 * Read a Register
		 * Command Format: p n
		 */
		case 'p':
			ptr_next += 1;
			token_expect_integer_arg(addr);

			if (addr >= DBG_CPU_NUM_REGISTERS) {
				goto error;
			}

			/* Read Register */
			status = dbg_enc_hex(pkt_buf, sizeof(pkt_buf),
			                     (char *)&(state->registers[addr]),
			                     sizeof(state->registers[addr]));
			if (status == EOF) {
				goto error;
			}
			dbg_send_packet(state, pkt_buf, status);
			break;
		
		/*
		 * Write a Register
		 * Command Format: P n...=r...
		 */
		case 'P':
			ptr_next += 1;
			token_expect_integer_arg(addr);
			token_expect_seperator('=');

			if (addr < DBG_CPU_NUM_REGISTERS) {
				status = dbg_dec_hex(ptr_next, token_remaining_buf,
				                     (char *)&(state->registers[addr]),
				                     sizeof(state->registers[addr]));
				if (status == EOF) {
					goto error;
				}
			}
			dbg_send_ok_packet(state, pkt_buf, sizeof(pkt_buf));
			break;
		
		/*
		 * Read Memory
		 * Command Format: m addr,length
		 */
		case 'm':
			ptr_next += 1;
			token_expect_integer_arg(addr);
			token_expect_seperator(',');
			token_expect_integer_arg(length);

			/* Read Memory */
			status = dbg_mem_read(state, pkt_buf, sizeof(pkt_buf),
			                      addr, length, dbg_enc_hex);
			if (status == EOF) {
				goto error;
			}
			dbg_send_packet(state, pkt_buf, status);
			break;
		
		/*
		 * Write Memory
		 * Command Format: M addr,length:XX..
		 */
		case 'M':
			ptr_next += 1;
			token_expect_integer_arg(addr);
			token_expect_seperator(',');
			token_expect_integer_arg(length);
			token_expect_seperator(':');

			/* Write Memory */
			status = dbg_mem_write(state, ptr_next, token_remaining_buf,
			                       addr, length, dbg_dec_hex);
			if (status == EOF) {
				goto error;
			}
			dbg_send_ok_packet(state, pkt_buf, sizeof(pkt_buf));
			break;

		/*
		 * Write Memory (Binary)
		 * Command Format: X addr,length:XX..
		 */
		case 'X':
			ptr_next += 1;
			token_expect_integer_arg(addr);
			token_expect_seperator(',');
			token_expect_integer_arg(length);
			token_expect_seperator(':');

			/* Write Memory */
			status = dbg_mem_write(state, ptr_next, token_remaining_buf,
			                       addr, length, dbg_dec_bin);
			if (status == EOF) {
				goto error;
			}
			dbg_send_ok_packet(state, pkt_buf, sizeof(pkt_buf));
			break;

		/* 
		 * Continue
		 * Command Format: c [addr]
		 */
		case 'c':
			dbg_continue(state);
			return 0;

		/*
		 * Single-step
		 * Command Format: s [addr]
		 */
		case 's':
			dbg_step(state);
			return 0;

		case '?':
			dbg_send_signal_packet(state, pkt_buf, sizeof(pkt_buf), state->signum);
			break;

		/*
		 * Unsupported Command
		 */
		default:
			dbg_send_packet(state, NULL, 0);
		}

		continue;

	error:
		dbg_send_error_packet(state, pkt_buf, sizeof(pkt_buf), 0x00);

		#undef token_remaining_buf
		#undef token_expect_seperator
		#undef token_expect_integer_arg
	}

	return 0;
}
