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

#include "quik.h"
#include "part.h"
#include "disk.h"
#include <mac-part.h>

static quik_err_t
read_mac_partition(ihandle dev,
                   unsigned part,
                   part_t *p)
{
   unsigned i;
   unsigned upart;
   unsigned blocks_in_map;
   length_t len;
   length_t secsize;
   char blk[SECTOR_SIZE];

   struct mac_partition *mp = (struct mac_partition *) blk;
   struct mac_driver_desc *md = (struct mac_driver_desc *) blk;

   len = disk_read(dev, blk, SECTOR_SIZE, 0LL);
   if (len != SECTOR_SIZE) {
      return ERR_DEV_SHORT_READ;
   }

   if (md->signature != MAC_DRIVER_MAGIC) {
      return ERR_PART_NOT_MAC;
   }

   secsize = md->block_size;
   blocks_in_map = 1;
   upart = 0;
   for (i = 1; i <= blocks_in_map; ++i) {
      if (disk_read(dev, blk, SECTOR_SIZE, (offset_t) i * secsize) != SECTOR_SIZE) {
         return ERR_DEV_SHORT_READ;
      }

      if (mp->signature != MAC_PARTITION_MAGIC) {
         break;
      }

      if (i == 1) {
         blocks_in_map = mp->map_count;
      }

      ++upart;

      /* If part is 0, use the first bootable partition. */
      if (part == upart
          || (part == 0 && (mp->status & STATUS_BOOTABLE) != 0
              && strcasecmp(mp->processor, "powerpc") == 0)) {
         p->start = (offset_t) mp->start_block * (offset_t) secsize;
         p->len = (offset_t) mp->block_count * (offset_t) secsize;
         p->dev = dev;
         return ERR_NONE;
      }
   }

   return ERR_PART_NOT_FOUND;
}

typedef struct {
   unsigned char boot_ind;   /* 0x80 - active */
   unsigned char head;    /* starting head */
   unsigned char sector;  /* starting sector */
   unsigned char cyl;     /* starting cylinder */
   unsigned char sys_ind; /* What partition type */
   unsigned char end_head;   /* end head */
   unsigned char end_sector; /* end sector */
   unsigned char end_cyl; /* end cylinder */
   unsigned int start_sect;  /* starting sector counting from 0 */
   unsigned int nr_sects; /* nr of sectors in partition */
} dos_part_t;


static quik_err_t
read_dos_partition(ihandle dev,
                   unsigned part,
                   part_t *p)
{
   length_t len;
   unsigned i;
   char blk[SECTOR_SIZE];
   dos_part_t *d;

   len = disk_read(dev, blk, SECTOR_SIZE, 0LL);
   if (len != SECTOR_SIZE) {
      return ERR_DEV_SHORT_READ;
   }

   /* check the MSDOS partition magic */
   if ((blk[0x1fe] != 0x55) || (blk[0x1ff] != 0xaa)) {
      return ERR_PART_NOT_DOS;
   }

   if (part >= 4) {
      return ERR_PART_NOT_FOUND;
   }

   d = (dos_part_t *) &blk[0x1be];
   for (i = 1; i != part ; i++, d++) {
      /* nothing */
   }

   p->dev = dev;
   p->start = swab32(d->start_sect) * SECTOR_SIZE;
   p->len = swab32(d->nr_sects) * SECTOR_SIZE;
   return ERR_NONE;
}


quik_err_t
part_open(char *device,
          int partno,
          part_t *part)
{
   ihandle dev;
   quik_err_t err;
   quik_err_t err2;

   if (part->flags & PART_VALID) {
      if (part->partno == partno &&
          !strcmp(device, part->devname)) {
         return ERR_NONE;
      }

      part_close(part);
   }

   err = disk_open(device, &dev);
   if (err != ERR_NONE) {
      return err;
   }

   err = read_mac_partition(dev, partno, part);
   if (err == ERR_PART_NOT_MAC) {
      err2 = read_dos_partition(dev, partno, part);

      if (err2 == ERR_PART_NOT_DOS) {
         err = ERR_PART_NOT_PARTITIONED;
      } else {
         err = err2;
      }
   }

   if (err != ERR_NONE) {
      disk_close(dev);
      return err;
   }

   part->partno = partno;
   strncpy(part->devname, device, sizeof(part->devname));
   part->flags |= PART_VALID;

   return ERR_NONE;
}


void
part_close(part_t *part)
{
   disk_close(part->dev);
   memset(part, 0, sizeof(*part));
}


quik_err_t
part_read(part_t *part,
          offset_t sector,
          offset_t byte_offset,
          length_t byte_len,
          char *buf)
{
   length_t len;
   offset_t off;

   off = sector << SECTOR_BITS;

   /*
    *  Check partition boundaries.
    */
   if ((off + byte_offset + byte_len) >=
       part->len) {
      return ERR_PART_BEYOND;
   }

   off += part->start + byte_offset;
   len = disk_read(part->dev, buf, byte_len, off);
   if (len != byte_len) {
      return ERR_DEV_SHORT_READ;
   }

   return ERR_NONE;
}
