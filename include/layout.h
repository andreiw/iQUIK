/*
 * layout.h - definitions for where the boot code goes in memory.
 *
 * Copyright (C) 1996 Paul Mackerras
 * Copyright (C) 2013 Andrei Warkentin <andrey.warkentin@gmail.com.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#define VERSION		"3.0"

#define SECOND_BASE	0x3e0000
#define SECOND_SIZE	0x10000

#define STACK_TOP	0x400000

/* 0x400000 - 0x500000 is one of OldWorld OF's favourite places to be. */
#define MALLOC_BASE	0x500000
