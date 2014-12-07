/*
 * Disk access.
 *
 * Copyright (C) 2013, Andrei Warkentin <andrey.warkentin@gmail.com>
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
#include "disk.h"

quik_err_t
disk_open(char *device,
          ihandle *dev)
{
   quik_err_t err;

   err = prom_open(device, dev);

   return err;
}


void
disk_close(ihandle dev)
{

   /*
    * Out params is indeed '0' or the close won't happen. Grrr...
    */
   call_prom("close", 1, 0, dev);
}


length_t
disk_read(ihandle dev,
          char *buf,
          length_t nbytes,
          offset_t offset)
{
   length_t nr;

   if (nbytes == 0) {
      return 0;
   }

   nr = (length_t) call_prom("seek", 3, 1, dev,
                             (unsigned int) (offset >> 32),
                             (unsigned int) (offset & 0xFFFFFFFF));

   nr = (length_t) call_prom("read", 3, 1, dev,
                             buf, nbytes);

   if (nr != nbytes) {
      printk("Read error at offset %x%x (%u instead of %u)\n",
             (uint32_t) (offset >> 32), (uint32_t) offset,
             nr, nbytes);
   }

   return nr;
}
