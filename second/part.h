/*
 * Disk partition access code.
 *
 * Copyright (C) 2013 Andrei Warkentin <andrey.warkentin@gmail.com>
 * (C) Copyright 2004
 * esd gmbh <www.esd-electronics.com>
 * Reinhard Arlt <reinhard.arlt@esd-electronics.com>
 *
 * based on code of fs/reiserfs/dev.c by
 *
 * (C) Copyright 2003 - 2004
 * Sysgo AG, <www.elinos.com>, Pavel Bartusek <pba@sysgo.com>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef QUIK_PART_H
#define QUIK_PART_H

#include "quik.h"
#include "prom.h"

typedef struct {
   int partno;
   ihandle  dev;
   offset_t start;
   offset_t len;
   unsigned flags;
#define PART_VALID   (1 << 0)
#define PART_MOUNTED (1 << 1)
   char devname[512];
} part_t;

quik_err_t part_open(char *device, int partno, part_t *part);
void part_close(part_t *part);
quik_err_t part_read(part_t *part,
                     offset_t sector,
                     offset_t byte_offset,
                     length_t byte_len,
                     char *buf);


#endif /* QUIK_PART_H */
