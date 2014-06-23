/*
 * Seven-Segment Driver
 *
 * Copyright (C) 2014 Thomas Kolb
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef SIGNALLING_H
#define SIGNALLING_H

extern size_t nseg;

void signalling_sendbit(int bit);
void signalling_commit(void);
void signalling_sendbyte(unsigned char byte);
void signalling_showtext(unsigned char *text, size_t len);
int  init_signalling(void);
void cleanup_signalling(void);

#endif // SIGNALLING_H
