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

#ifndef GDBSTUB_H
#define GDBSTUB_H

/* Enable debug statements (printf) */
#ifndef DEBUG
#define DEBUG 0
#endif

/*****************************************************************************
 *
 *  Mock
 *
 ****************************************************************************/

#ifdef GDBSTUB_ARCH_MOCK

#include <string.h>

/*****************************************************************************
 * Types
 ****************************************************************************/

typedef unsigned int address;
typedef unsigned int reg;

enum DBG_REGISTER {
    DBG_CPU_NUM_REGISTERS = 4
};

struct dbg_state {
    int signum;
    reg registers[DBG_CPU_NUM_REGISTERS];
};

/*****************************************************************************
 * Prototypes
 ****************************************************************************/

struct dbg_buffer;
extern struct dbg_buffer dbg_input, dbg_output;

void dbg_buf_write(struct dbg_buffer *buf, int ch);
int dbg_buf_read(struct dbg_buffer *buf);

#endif /* GDBSTUB_ARCH_MOCK */

/*****************************************************************************
 *
 *  x86
 *
 ****************************************************************************/

#ifdef GDBSTUB_ARCH_X86

#define DBG_DEFINE_SIZET_TYPE 1
#define DBG_DEFINE_STDINT_TYPES 1

/*****************************************************************************
 * Types
 ****************************************************************************/

#if DBG_DEFINE_STDINT_TYPES
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long  uint32_t;
#endif

#if DBG_DEFINE_SIZET_TYPE
typedef unsigned int size_t;
#endif

typedef unsigned int address;
typedef unsigned int reg;

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

#endif /* GDBSTUB_ARCH_X86 */

/*****************************************************************************
 *
 *  GDB Remote Serial Protocol
 *
 ****************************************************************************/

/*****************************************************************************
 * Macros
 ****************************************************************************/

#if DEBUG
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

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
void dbg_sys_init(void);
int dbg_sys_getc(struct dbg_state *state);
int dbg_sys_putchar(struct dbg_state *state, int ch);
int dbg_sys_mem_readb(struct dbg_state *state, address addr, char *val);
int dbg_sys_mem_writeb(struct dbg_state *state, address addr, char val);
int dbg_sys_continue(struct dbg_state *state);
int dbg_sys_step(struct dbg_state *state);

#ifdef GDBSTUB_IMPLEMENTATION

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
        if (tmp == DBG_EOF) {
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
        return DBG_EOF;
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
        return DBG_EOF;
    }

    return (value < base) ? value : DBG_EOF;
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
 *    DBG_EOF otherwise
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
        DBG_PRINT("received bad packet response: 0x%2x\n", response);
        return DBG_EOF;
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
 *    DBG_EOF otherwise
 */
int dbg_send_packet(struct dbg_state *state, const char *pkt_data,
                    size_t pkt_len)
{
    char buf[3];
    char csum;

    /* Send packet start */
    if (dbg_sys_putchar(state, '$') == DBG_EOF) {
        return DBG_EOF;
    }

#if DEBUG
    {
        size_t p;
        DBG_PRINT("-> ");
        for (p = 0; p < pkt_len; p++) {
            if (dbg_is_printable_char(pkt_data[p])) {
                DBG_PRINT("%c", pkt_data[p]);
            } else {
                DBG_PRINT("\\x%02x", pkt_data[p]&0xff);
            }
        }
        DBG_PRINT("\n");
    }
#endif

    /* Send packet data */
    if (dbg_write(state, pkt_data, pkt_len) == DBG_EOF) {
        return DBG_EOF;
    }

    /* Send the checksum */
    buf[0] = '#';
    csum = dbg_checksum(pkt_data, pkt_len);
    if ((dbg_enc_hex(buf+1, sizeof(buf)-1, &csum, 1) == DBG_EOF) ||
        (dbg_write(state, buf, sizeof(buf)) == DBG_EOF)) {
        return DBG_EOF;
    }

    return dbg_recv_ack(state);
}

/*
 * Receives a packet of data, assuming a 7-bit clean connection.
 *
 * Returns:
 *    0   if the packet was received
 *    DBG_EOF otherwise
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
        if (data == DBG_EOF) {
            return DBG_EOF;
        } else if (data == '$') {
            /* Detected start of packet. */
            break;
        }
    }

    /* Read until checksum */
    *pkt_len = 0;
    while (1) {
        data = dbg_sys_getc(state);

        if (data == DBG_EOF) {
            /* Error receiving character */
            return DBG_EOF;
        } else if (data == '#') {
            /* End of packet */
            break;
        } else {
            /* Check for space */
            if (*pkt_len >= pkt_buf_len) {
                DBG_PRINT("packet buffer overflow\n");
                return DBG_EOF;
            }

            /* Store character and update checksum */
            pkt_buf[(*pkt_len)++] = (char) data;
        }
    }

#if DEBUG
    {
        size_t p;
        DBG_PRINT("<- ");
        for (p = 0; p < *pkt_len; p++) {
            if (dbg_is_printable_char(pkt_buf[p])) {
                DBG_PRINT("%c", pkt_buf[p]);
            } else {
                DBG_PRINT("\\x%02x", pkt_buf[p] & 0xff);
            }
        }
        DBG_PRINT("\n");
    }
#endif

    /* Receive the checksum */
    if ((dbg_read(state, buf, sizeof(buf), 2) == DBG_EOF) ||
        (dbg_dec_hex(buf, 2, &expected_csum, 1) == DBG_EOF)) {
        return DBG_EOF;
    }

    /* Verify checksum */
    actual_csum = dbg_checksum(pkt_buf, *pkt_len);
    if (actual_csum != expected_csum) {
        /* Send packet nack */
        DBG_PRINT("received packet with bad checksum\n");
        dbg_sys_putchar(state, '-');
        return DBG_EOF;
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
 *    DBG_EOF if the buffer is too small
 */
int dbg_enc_hex(char *buf, size_t buf_len, const char *data, size_t data_len)
{
    size_t pos;

    if (buf_len < data_len*2) {
        /* Buffer too small */
        return DBG_EOF;
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
 *    DBG_EOF if the buffer is too small
 */
int dbg_dec_hex(const char *buf, size_t buf_len, char *data, size_t data_len)
{
    size_t pos;
    int tmp;

    if (buf_len != data_len*2) {
        /* Buffer too small */
        return DBG_EOF;
    }

    for (pos = 0; pos < data_len; pos++) {
        /* Decode high nibble */
        tmp = dbg_get_val(*buf++, 16);
        if (tmp == DBG_EOF) {
            /* Buffer contained junk. */
            DBG_ASSERT(0);
            return DBG_EOF;
        }

        data[pos] = tmp << 4;

        /* Decode low nibble */
        tmp = dbg_get_val(*buf++, 16);
        if (tmp == DBG_EOF) {
            /* Buffer contained junk. */
            DBG_ASSERT(0);
            return DBG_EOF;
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
 *    DBG_EOF if the buffer is too small
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
                DBG_ASSERT(0);
                return DBG_EOF;
            }
            buf[buf_pos++] = '}';
            buf[buf_pos++] = data[data_pos] ^ 0x20;
        } else {
            if (buf_pos >= buf_len) {
                DBG_ASSERT(0);
                return DBG_EOF;
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
 *    DBG_EOF if the buffer is too small
 */
int dbg_dec_bin(const char *buf, size_t buf_len, char *data, size_t data_len)
{
    size_t buf_pos, data_pos;

    for (buf_pos = 0, data_pos = 0; buf_pos < buf_len; buf_pos++) {
        if (data_pos >= data_len) {
            /* Output buffer overflow */
            DBG_ASSERT(0);
            return DBG_EOF;
        }
        if (buf[buf_pos] == '}') {
            /* The next byte is escaped! */
            if (buf_pos+1 >= buf_len) {
                /* There's an escape character, but no escaped character
                 * following the escape character. */
                DBG_ASSERT(0);
                return DBG_EOF;
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
 *    DBG_EOF if the buffer is too small
 */
int dbg_mem_read(struct dbg_state *state, char *buf, size_t buf_len,
                 address addr, size_t len, dbg_enc_func enc)
{
    char data[64];
    size_t pos;

    if (len > sizeof(data)) {
        return DBG_EOF;
    }

    /* Read from system memory */
    for (pos = 0; pos < len; pos++) {
        if (dbg_sys_mem_readb(state, addr+pos, &data[pos])) {
            /* Failed to read */
            return DBG_EOF;
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
        return DBG_EOF;
    }

    /* Decode data */
    if (dec(buf, buf_len, data, len) == DBG_EOF) {
        return DBG_EOF;
    }

    /* Write to system memory */
    for (pos = 0; pos < len; pos++) {
        if (dbg_sys_mem_writeb(state, addr+pos, data[pos])) {
            /* Failed to write */
            return DBG_EOF;
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
        return DBG_EOF;
    }

    buf[0] = 'O';
    status = dbg_enc_hex(&buf[1], buf_len-1, msg, dbg_strlen(msg));
    if (status == DBG_EOF) {
        return DBG_EOF;
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
        return DBG_EOF;
    }

    buf[0] = 'S';
    status = dbg_enc_hex(&buf[1], buf_len-1, &signal, 1);
    if (status == DBG_EOF) {
        return DBG_EOF;
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
        return DBG_EOF;
    }

    buf[0] = 'E';
    status = dbg_enc_hex(&buf[1], buf_len-1, &error, 1);
    if (status == DBG_EOF) {
        return DBG_EOF;
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
 *    DBG_EOF if failed to write all bytes
 */
int dbg_write(struct dbg_state *state, const char *buf, size_t len)
{
    while (len--) {
        if (dbg_sys_putchar(state, *buf++) == DBG_EOF) {
            return DBG_EOF;
        }
    }

    return 0;
}

/*
 * Read a sequence of bytes.
 *
 * Returns:
 *    0   if successfully read len bytes
 *    DBG_EOF if failed to read all bytes
 */
int dbg_read(struct dbg_state *state, char *buf, size_t buf_len, size_t len)
{
    char c;

    if (buf_len < len) {
        /* Buffer too small */
        return DBG_EOF;
    }

    while (len--) {
        if ((c = dbg_sys_getc(state)) == DBG_EOF) {
            return DBG_EOF;
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
        if (status == DBG_EOF) {
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
            if (status == DBG_EOF) {
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
            if (status == DBG_EOF) {
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
            if (status == DBG_EOF) {
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
                if (status == DBG_EOF) {
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
            if (status == DBG_EOF) {
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
            if (status == DBG_EOF) {
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
            if (status == DBG_EOF) {
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
            dbg_send_signal_packet(state, pkt_buf, sizeof(pkt_buf),
                                   state->signum);
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

/*****************************************************************************
 *
 *  Mock
 *
 ****************************************************************************/

#ifdef GDBSTUB_ARCH_MOCK

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

#endif /* GDBSTUB_ARCH_MOCK */


/*****************************************************************************
 *
 *  x86
 *
 ****************************************************************************/

#ifdef GDBSTUB_ARCH_X86

/*****************************************************************************
 * Types
 ****************************************************************************/

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

/*****************************************************************************
 * Const Data
 ****************************************************************************/

extern void const * const dbg_int_handlers[];

/*****************************************************************************
 * Prototypes
 ****************************************************************************/

void dbg_x86_hook_idt(uint8_t vector, const void *function);
void dbg_x86_init_gates(void);
void dbg_x86_init_idt(void);
void dbg_x86_load_idt(struct dbg_idtr *idtr);
void dbg_x86_store_idt(struct dbg_idtr *idtr);
uint32_t dbg_x86_get_cs(void);
void dbg_x86_int_handler(struct dbg_interrupt_state *istate);
void dbg_x86_interrupt(struct dbg_interrupt_state *istate);
void dbg_x86_io_write_8(uint16_t port, uint8_t val);
uint8_t dbg_x86_io_read_8(uint16_t port);
void *dbg_x86_sys_memset(void *ptr, int data, size_t len);

#ifdef __STRICT_ANSI__
#define asm __asm__
#endif

#define SERIAL_COM1 0x3f8
#define SERIAL_COM2 0x2f8
#define SERIAL_PORT SERIAL_COM1

#define NUM_IDT_ENTRIES 32

/*****************************************************************************
 * BSS Data
 ****************************************************************************/

static struct dbg_idt_gate dbg_idt_gates[NUM_IDT_ENTRIES];
static struct dbg_state    dbg_state;

/*****************************************************************************
 * Misc. Functions
 ****************************************************************************/

void *dbg_x86_sys_memset(void *ptr, int data, size_t len)
{
    char *p = ptr;

    while (len--) {
        *p++ = (char)data;
    }

    return ptr;
}

/*
 * Get current code segment (CS register).
 */
uint32_t dbg_x86_get_cs(void)
{
    uint32_t cs;

    asm volatile (
        "push    %%cs;"
        "pop     %%eax;"
        /* Outputs  */ : "=a" (cs)
        /* Inputs   */ : /* None */
        /* Clobbers */ : /* None */
        );

    return cs;
}

/*****************************************************************************
 * Interrupt Management Functions
 ****************************************************************************/

/*
 * Initialize idt_gates with the interrupt handlers.
 */
void dbg_x86_init_gates(void)
{
    size_t   i;
    uint16_t cs;

    cs = dbg_x86_get_cs();
    for (i = 0; i < NUM_IDT_ENTRIES; i++) {
        dbg_idt_gates[i].flags       = 0x8E00;
        dbg_idt_gates[i].segment     = cs;
        dbg_idt_gates[i].offset_low  =
            ((uint32_t)dbg_int_handlers[i]      ) & 0xffff;
        dbg_idt_gates[i].offset_high =
            ((uint32_t)dbg_int_handlers[i] >> 16) & 0xffff;
    }
}

/*
 * Load a new IDT.
 */
void dbg_x86_load_idt(struct dbg_idtr *idtr)
{
    asm volatile (
        "lidt    %0"
        /* Outputs  */ : /* None */
        /* Inputs   */ : "m" (*idtr)
        /* Clobbers */ : /* None */
        );
}

/*
 * Get current IDT.
 */
void dbg_x86_store_idt(struct dbg_idtr *idtr)
{
    asm volatile (
        "sidt    %0"
        /* Outputs  */ : "=m" (*idtr)
        /* Inputs   */ : /* None */
        /* Clobbers */ : /* None */
        );
}

/*
 * Hook a vector of the current IDT.
 */
void dbg_x86_hook_idt(uint8_t vector, const void *function)
{
    struct dbg_idtr      idtr;
    struct dbg_idt_gate *gates;

    dbg_x86_store_idt(&idtr);
    gates = (struct dbg_idt_gate *)idtr.offset;
    gates[vector].flags       = 0x8E00;
    gates[vector].segment     = dbg_x86_get_cs();
    gates[vector].offset_low  = (((uint32_t)function)      ) & 0xffff;
    gates[vector].offset_high = (((uint32_t)function) >> 16) & 0xffff;
}

/*
 * Initialize IDT gates and load the new IDT.
 */
void dbg_x86_init_idt(void)
{
    struct dbg_idtr idtr;

    dbg_x86_init_gates();
    idtr.len = sizeof(dbg_idt_gates)-1;
    idtr.offset = (uint32_t)dbg_idt_gates;
    dbg_x86_load_idt(&idtr);
}

/*
 * Common interrupt handler routine.
 */
void dbg_x86_int_handler(struct dbg_interrupt_state *istate)
{
    dbg_x86_interrupt(istate);
}

/*
 * Debug interrupt handler.
 */
void dbg_x86_interrupt(struct dbg_interrupt_state *istate)
{
    dbg_x86_sys_memset(&dbg_state.registers, 0, sizeof(dbg_state.registers));

    /* Translate vector to signal */
    switch (istate->vector) {
    case 1:  dbg_state.signum = 5; break;
    case 3:  dbg_state.signum = 5; break;
    default: dbg_state.signum = 7;
    }

    /* Load Registers */
    dbg_state.registers[DBG_CPU_I386_REG_EAX] = istate->eax;
    dbg_state.registers[DBG_CPU_I386_REG_ECX] = istate->ecx;
    dbg_state.registers[DBG_CPU_I386_REG_EDX] = istate->edx;
    dbg_state.registers[DBG_CPU_I386_REG_EBX] = istate->ebx;
    dbg_state.registers[DBG_CPU_I386_REG_ESP] = istate->esp;
    dbg_state.registers[DBG_CPU_I386_REG_EBP] = istate->ebp;
    dbg_state.registers[DBG_CPU_I386_REG_ESI] = istate->esi;
    dbg_state.registers[DBG_CPU_I386_REG_EDI] = istate->edi;
    dbg_state.registers[DBG_CPU_I386_REG_PC]  = istate->eip;
    dbg_state.registers[DBG_CPU_I386_REG_CS]  = istate->cs;
    dbg_state.registers[DBG_CPU_I386_REG_PS]  = istate->eflags;
    dbg_state.registers[DBG_CPU_I386_REG_SS]  = istate->ss;
    dbg_state.registers[DBG_CPU_I386_REG_DS]  = istate->ds;
    dbg_state.registers[DBG_CPU_I386_REG_ES]  = istate->es;
    dbg_state.registers[DBG_CPU_I386_REG_FS]  = istate->fs;
    dbg_state.registers[DBG_CPU_I386_REG_GS]  = istate->gs;

    dbg_main(&dbg_state);

    /* Restore Registers */
    istate->eax    = dbg_state.registers[DBG_CPU_I386_REG_EAX];
    istate->ecx    = dbg_state.registers[DBG_CPU_I386_REG_ECX];
    istate->edx    = dbg_state.registers[DBG_CPU_I386_REG_EDX];
    istate->ebx    = dbg_state.registers[DBG_CPU_I386_REG_EBX];
    istate->esp    = dbg_state.registers[DBG_CPU_I386_REG_ESP];
    istate->ebp    = dbg_state.registers[DBG_CPU_I386_REG_EBP];
    istate->esi    = dbg_state.registers[DBG_CPU_I386_REG_ESI];
    istate->edi    = dbg_state.registers[DBG_CPU_I386_REG_EDI];
    istate->eip    = dbg_state.registers[DBG_CPU_I386_REG_PC];
    istate->cs     = dbg_state.registers[DBG_CPU_I386_REG_CS];
    istate->eflags = dbg_state.registers[DBG_CPU_I386_REG_PS];
    istate->ss     = dbg_state.registers[DBG_CPU_I386_REG_SS];
    istate->ds     = dbg_state.registers[DBG_CPU_I386_REG_DS];
    istate->es     = dbg_state.registers[DBG_CPU_I386_REG_ES];
    istate->fs     = dbg_state.registers[DBG_CPU_I386_REG_FS];
    istate->gs     = dbg_state.registers[DBG_CPU_I386_REG_GS];
}

/*****************************************************************************
 * I/O Functions
 ****************************************************************************/

/*
 * Write to I/O port.
 */
void dbg_x86_io_write_8(uint16_t port, uint8_t val)
{
    asm volatile (
        "outb    %%al, %%dx;"
        /* Outputs  */ : /* None */
        /* Inputs   */ : "a" (val), "d" (port)
        /* Clobbers */ : /* None */
        );
}

/*
 * Read from I/O port.
 */
uint8_t dbg_x86_io_read_8(uint16_t port)
{
    uint8_t val;

    asm volatile (
        "inb     %%dx, %%al;"
        /* Outputs  */ : "=a" (val)
        /* Inputs   */ : "d" (port)
        /* Clobbers */ : /* None */
        );

    return val;
}

/*****************************************************************************
 * NS16550 Serial Port (IO)
 ****************************************************************************/

#define SERIAL_THR 0
#define SERIAL_RBR 0
#define SERIAL_LSR 5

int dbg_serial_getc(void)
{
    /* Wait for data */
    while ((dbg_x86_io_read_8(SERIAL_PORT + SERIAL_LSR) & 1) == 0);
    return dbg_x86_io_read_8(SERIAL_PORT + SERIAL_RBR);
}

int dbg_serial_putchar(int ch)
{
    /* Wait for THRE (bit 5) to be high */
    while ((dbg_x86_io_read_8(SERIAL_PORT + SERIAL_LSR) & (1<<5)) == 0);
    dbg_x86_io_write_8(SERIAL_PORT + SERIAL_THR, ch);
    return ch;
}

/*****************************************************************************
 * Debugging System Functions
 ****************************************************************************/

/*
 * Write one character to the debugging stream.
 */
int dbg_sys_putchar(struct dbg_state *state, int ch)
{
    return dbg_serial_putchar(ch);
}

/*
 * Read one character from the debugging stream.
 */
int dbg_sys_getc(struct dbg_state *state)
{
    return dbg_serial_getc() & 0xff;
}

/*
 * Read one byte from memory.
 */
int dbg_sys_mem_readb(struct dbg_state *state, address addr, char *val)
{
    *val = *(volatile char *)addr;
    return 0;
}

/*
 * Write one byte to memory.
 */
int dbg_sys_mem_writeb(struct dbg_state *state, address addr, char val)
{
    *(volatile char *)addr = val;
    return 0;
}

/*
 * Continue program execution.
 */
int dbg_sys_continue(struct dbg_state *state)
{
    dbg_state.registers[DBG_CPU_I386_REG_PS] &= ~(1<<8);
    return 0;
}

/*
 * Single step the next instruction.
 */
int dbg_sys_step(struct dbg_state *state)
{
    dbg_state.registers[DBG_CPU_I386_REG_PS] |= 1<<8;
    return 0;
}

/*
 * Debugger init function.
 *
 * Hooks the IDT to enable debugging.
 */
void dbg_sys_init(void)
{
    /* Hook current IDT. */
    dbg_x86_hook_idt(1, dbg_int_handlers[1]);
    dbg_x86_hook_idt(3, dbg_int_handlers[3]);

    /* Interrupt to start debugging. */
    asm volatile ("int3");
}

#endif /* GDBSTUB_ARCH_X86 */
#endif /* GDBSTUB_IMPLEMENTATION */
#endif /* GDBSTUB_H */
