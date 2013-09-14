/*
 * Copyright (C) 2013 Andrei Warkentin <andrey.warkentin@gmail.com>
 * (C) Copyright 2004
 * esd gmbh <www.esd-electronics.com>
 * Reinhard Arlt <reinhard.arlt@esd-electronics.com>
 *
 * based on code from grub2 fs/ext2.c and fs/fshelp.c by
 *
 * GRUB  --  GRand Unified Bootloader
 * Copyright (C) 2003, 2004  Free Software Foundation, Inc.
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

#ifndef QUIK_EXT2FS_H
#define QUIK_EXT2FS_H

#include "quik.h"
#include "part.h"

void ext2fs_close(void);
quik_err_t ext2fs_read(char *buf, length_t len);
quik_err_t ext2fs_mount(part_t *part);
quik_err_t ext2fs_open(char *filename, length_t *out_len);
quik_err_t ext2fs_ls(char *dir);

#endif /* QUIK_EXT2FS_H */
