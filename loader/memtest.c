/*
 * Simple and terrible mem tester.
 *
 * Caveat emptor: you need to know the range you're testing, and
 * make sure this doesn't collide with anything else that's in use.
 *
 * Copyright (C) 2016 Andrei Warkentin <andrey.warkentin@gmail.com>
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
#include "prom.h"
#include "commands.h"


static quik_err_t
memtest(char *args)
{
   uint32_t base = 0;
   uint32_t size = 0;
   char *p1 = NULL;
   char *p2 = NULL;
   vaddr_t *buf = NULL;
   vaddr_t *buf_end = NULL;
   vaddr_t *p = NULL;

   printk("memtest args: '%s'\n", args);
   base = strtol(args, &p1, 0);
   if (p1 == args) {
      return ERR_CMD_BAD_PARAM;
   }
  
   size = strtol(p1, &p2, 0);
   if (p2 == p1) {
      return ERR_CMD_BAD_PARAM;
   }

   base = ALIGN_UP(base, SIZE_1M);
   size = ALIGN_UP(size, SIZE_1M);
   printk("mem test range 0x%x-0x%x\n",
          base, base + size);

   buf = prom_claim((void *) base, size);
   if (buf == (void *) -1) {
      return ERR_NO_MEM;
   }

   buf_end = (void *) buf + size;

   p = buf;
   while (p < buf_end) {
      if ((vaddr_t) p % SIZE_1M == 0) {
         printk("Writing 0x%x...\n", p);
      }
      *p = (vaddr_t) p;
      p++;
   }

   p = buf;
   while (p < buf_end) {
      if ((vaddr_t) p % SIZE_1M == 0) {
         printk("Reading 0x%x...\n", p);
      }
      if (*p != (vaddr_t) p) {
         printk("bad address 0x%x, got 0x%x\n", p, *p);
      }

      p++;
   }

   prom_release(buf, size);
   return ERR_NONE;
}

COMMAND(memtest, memtest, "simple mem tester");
