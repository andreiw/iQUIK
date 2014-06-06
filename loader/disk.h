/*
 * Disk access code.
 *
 * Copyright (C) 2013 Andrei Warkentin <andrey.warkentin@gmail.com>
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

#ifndef QUIK_DISK_H
#define QUIK_DISK_H

#include "quik.h"
#include "prom.h"

#define SECTOR_SIZE 512
#define SECTOR_BITS 9

quik_err_t disk_open(char *device, ihandle *dev);
void disk_close(ihandle dev);
length_t disk_read(ihandle dev,
                   char *buf,
                   length_t nbytes,
                   offset_t offset);

#endif /* QUIK_PART_H */
