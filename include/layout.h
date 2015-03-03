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

#define VERSION        "3.0"

/*
 * First byte (BE) must be NUL, the rest is used
 * to correctly position by the installer.
 */
#define PREBOOT_MAGIC   0x00B007ED

/*
 * Boot code load address and max size.
 */
#define IQUIK_BASE      0x3e0000
#define IQUIK_SIZE      0x20000

/*
 * 0x400000 - 0x500000 is one of OldWorld OF's favourite places to be,
 * so avoid...
 */
#define MALLOC_BASE     0x500000
#define MALLOC_SIZE     0x300000

/*
 * Anything loaded by iquik gets put above this address.
 */
#define LOAD_BASE       0x800000
