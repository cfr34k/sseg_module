/*
 * Seven-Segment Driver
 *
 * Copyright (C) 2014 Thomas Kolb
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define __NO_VERSION__

#include <linux/gpio.h>
#include <linux/ctype.h>

#include "signalling.h"
#include "config.h"

// number of segments
size_t nseg = 5;

// GPIOs
const int GPIO_DATA   = 9;
const int GPIO_CLOCK  = 8;
const int GPIO_STROBE = 6;
const int GPIO_OE     = 4;

#define NCHARS 47
const unsigned char PATTERNS[NCHARS] = {
	      //  i c 1234567.  charcode
	0x7E, //  0 0 01111110  '0'
	0x12, //  1 1 00010010  '1'
	0xBC, //  2 2 10111100  '2'
	0xB6, //  3 3 10110110  '3'
	0xD2, //  4 4 11010010  '4'
	0xE6, //  5 5 11100110  '5'
	0xEE, //  6 6 11101110  '6'
	0x32, //  7 7 00110010  '7'
	0xFE, //  8 8 11111110  '8'
	0xF6, //  9 9 11110110  '9'

	0xFA, // 10 A 11111010  'A' or 'a'
	0xCE, // 11 B 11001110  'B' or 'b'
	0x6C, // 12 C 01101100  'C' or 'c'
	0x9E, // 13 D 10011110  'D' or 'd'
	0xEC, // 14 E 11101100  'E' or 'e'
	0xE8, // 15 F 11101000  'F' or 'f'
	0x6E, // 16 G 01101110  'G' or 'g'
	0xDA, // 17 H 11011010  'H' or 'h'
	0x48, // 18 I 01001000  'I' or 'i'
	0x1E, // 19 J 00011110  'J' or 'j'
	0xEA, // 20 K 11101010  'K' or 'k'
	0x4C, // 21 L 01001100  'L' or 'l'
	0x7A, // 22 M 01111010  'M' or 'm'
	0x8A, // 23 N 10001010  'N' or 'n'
	0x8E, // 24 O 10001110  'O' or 'o'
	0xF8, // 25 P 11111000  'P' or 'p'
	0xF2, // 26 Q 11110010  'Q' or 'q'
	0x88, // 27 R 10001000  'R' or 'r'
	0xE6, // 28 S 11100110  'S' or 's'
	0xCC, // 29 T 11001100  'T' or 't'
	0x0E, // 30 U 00001110  'U' or 'u'
	0x5E, // 31 V 01011110  'V' or 'v'
	0xDE, // 32 W 11011110  'W' or 'w'
	0xDA, // 33 X 11011010  'X' or 'x'
	0xD2, // 34 Y 11010010  'Y' or 'y'
	0xBC, // 35 Z 10111100  'Z' or 'z'

	0x00, // 36   00000000  ' ' or '\t'

	// special (non-ascii) symbols
	0xC8, // 37 play  11001000  0x80
	0x5A, // 38 pause 01011010  0x81
	0xF0, // 39 stop  11110000  0x82

	// single segments
	0x80, // 40 cc    10000000  0x83
	0x20, // 41 tc    00100000  0x84
	0x10, // 42 tr    00010000  0x85
	0x02, // 43 br    00000010  0x86
	0x04, // 44 bc    00000100  0x87
	0x08, // 45 bl    00001000  0x88
	0x40, // 46 tl    01000000  0x89
};

void signalling_sendbit(int bit)
{
	// set clock low and update signals
	gpio_set_value(GPIO_CLOCK, 0);

	if(bit) {
		gpio_set_value(GPIO_DATA, 1);
	} else {
		gpio_set_value(GPIO_DATA, 0);
	}

	// set clock high to update shift registers
	gpio_set_value(GPIO_CLOCK, 1);
}

void signalling_commit(void)
{
	gpio_set_value(GPIO_STROBE, 1);
	gpio_set_value(GPIO_STROBE, 0);
}

void signalling_sendbyte(unsigned char byte)
{
	unsigned i;
	for(i = 0; i < 8; i++) {
		signalling_sendbit( (byte >> i) & 0x01 );
	}
}

void signalling_showtext(unsigned char *text, size_t len)
{
	int idx;
	unsigned char c;
	int strpos = len - 1;
	int digitsSent = 0;
	int includeDot = 0;

	while(digitsSent < nseg && strpos >= 0) {
		c = text[strpos];

		if(c == '.' || c == ':') {
			includeDot = 1;
			strpos--;
			continue;
		}

		if(isdigit(c)) {
			idx = c - '0';
		} else if(islower(c)) {
			idx = c - 'a' + 10;
		} else if(isupper(c)) {
			idx = c - 'A' + 10;
		} else if(isspace(c)) {
			idx = 36;
		} else if(c >= 0x80 && c < (0x80 + NCHARS - 36)) {
			// special characters starting from idx = 37
			idx = 37 + (c - 0x80);
		} else {
			// unsupported character
			strpos--;
			continue;
		}

		signalling_sendbyte( PATTERNS[idx] | (includeDot ? 0x01 : 0x00) );

		includeDot = 0;
		digitsSent++;
		strpos--;
	}

	// fill it up to 5 digits
	for(; digitsSent < nseg; digitsSent++) {
		signalling_sendbyte(PATTERNS[36]);
	}

#if DEBUG
	printk(KERN_ALERT "sseg: Sent %d digits", digitsSent);
#endif // DEBUG

	signalling_commit();
}

int init_signalling(void)
{
	// check for valid GPIOs
	if(!gpio_is_valid(GPIO_OE)) {
#if DEBUG
		printk(KERN_ALERT "sseg: OE GPIO is not valid!");
#endif // DEBUG
		goto fail;
	}

	if(!gpio_is_valid(GPIO_STROBE)) {
#if DEBUG
		printk(KERN_ALERT "sseg: STROBE GPIO is not valid!");
#endif // DEBUG
		goto fail;
	}

	if(!gpio_is_valid(GPIO_DATA)) {
#if DEBUG
		printk(KERN_ALERT "sseg: DATA GPIO is not valid!");
#endif // DEBUG
		goto fail;
	}

	if(!gpio_is_valid(GPIO_CLOCK)) {
#if DEBUG
		printk(KERN_ALERT "sseg: CLOCK GPIO is not valid!");
#endif // DEBUG
		goto fail;
	}

	// request GPIOs
	if(0 != gpio_request(GPIO_OE, "sseg_oe")) {
#if DEBUG
		printk(KERN_ALERT "sseg: OE GPIO cannot be requested!");
#endif // DEBUG
		goto fail_request;
	}

	if(0 != gpio_request(GPIO_STROBE, "sseg_strobe")) {
#if DEBUG
		printk(KERN_ALERT "sseg: STROBE GPIO cannot be requested!");
#endif // DEBUG
		goto fail_request;
	}

	if(0 != gpio_request(GPIO_DATA, "sseg_data")) {
#if DEBUG
		printk(KERN_ALERT "sseg: DATA GPIO cannot be requested!");
#endif // DEBUG
		goto fail_request;
	}

	if(0 != gpio_request(GPIO_CLOCK, "sseg_clock")) {
#if DEBUG
		printk(KERN_ALERT "sseg: CLOCK GPIO cannot be requested!");
#endif // DEBUG
		goto fail_request;
	}

	// set GPIO direction
	if(0 != gpio_direction_output(GPIO_OE, 1)) {
#if DEBUG
		printk(KERN_ALERT "sseg: Cannot set direction of OE");
#endif // DEBUG
		goto fail_direction;
	}

	if(0 != gpio_direction_output(GPIO_STROBE, 0)) {
#if DEBUG
		printk(KERN_ALERT "sseg: Cannot set direction of STROBE");
#endif // DEBUG
		goto fail_direction;
	}

	if(0 != gpio_direction_output(GPIO_DATA, 0)) {
#if DEBUG
		printk(KERN_ALERT "sseg: Cannot set direction of DATA");
#endif // DEBUG
		goto fail_direction;
	}

	if(0 != gpio_direction_output(GPIO_CLOCK, 0)) {
#if DEBUG
		printk(KERN_ALERT "sseg: Cannot set direction of CLOCK");
#endif // DEBUG
		goto fail_direction;
	}

	return 0; // success

fail_direction:
fail_request:
	gpio_free(GPIO_OE);
	gpio_free(GPIO_STROBE);
	gpio_free(GPIO_DATA);
	gpio_free(GPIO_CLOCK);
fail:
	return -1;
}

void cleanup_signalling(void)
{
	gpio_free(GPIO_OE);
	gpio_free(GPIO_STROBE);
	gpio_free(GPIO_DATA);
	gpio_free(GPIO_CLOCK);
}
