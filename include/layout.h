/*
 * layout.h - definitions for where the 1st and 2nd stage bootstraps
 * go in memory and the structures they use to communicate.
 *
 * Copyright (C) 1996 Paul Mackerras
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#define VERSION		"2.0"

#define FIRST_BASE	0x3f4000
#define FIRST_SIZE	0x400

#define SECOND_BASE	0x3e0000
#define SECOND_SIZE	0x10000

#define STACK_TOP	0x400000

/* 0x400000 - 0x500000 is one of OF's favourite places to be. */

#define MALLOC_BASE	0x500000

#ifndef __ASSEMBLY__
struct first_info {
    char	quik_vers[8];
    int		nblocks;
    int		blocksize;
    unsigned	second_base;
    int		conf_part;
    char	conf_file[32];
    unsigned	blknos[64];
};
#endif	/* __ASSEMBLY__ */

/*
 * If you change FIRST_SIZE or sizeof(struct first_info),
 * be sure to change the definition of FIRST_INFO_OFF.
 */
#define FIRST_INFO_OFF	0x2c8	/* == FIRST_SIZE - sizeof(struct first_info) */
#define FIRST_INFO	(FIRST_BASE + FIRST_INFO_OFF)
