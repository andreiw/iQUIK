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
   ihandle current_dev;

   current_dev = call_prom("open", 1, 1, device);
   if (current_dev == (ihandle) 0 || current_dev == (ihandle) -1) {
      return ERR_DEV_OPEN;
   }

   *dev = current_dev;
   return ERR_NONE;
}


void
disk_close(ihandle dev)
{

   /*
    * Out params is indeed '0' or the close won't happen. Grrr...
    */
   call_prom("close", 1, 0, dev);
}


void
disk_init(boot_info_t *bi)
{
   char *p;

   if (bi->flags & WITH_PREBOOT)  {

      /*
       * If we ran a preboot script, ignore all
       * passed parameters and treat the set
       * boot-file as bootargs.
       */
      prom_get_options("boot-file", bi->of_bootargs, sizeof(bi->of_bootargs));
      bi->bootargs = bi->of_bootargs;
   }

   bi->default_device = bi->bootargs;
   word_split(&bi->default_device, &bi->bootargs);

   if (!bi->default_device) {
      prom_get_options("boot-file", bi->bootfile, sizeof(bi->bootfile));

      if (bi->flags & WITH_PREBOOT) {
         printk("Preboot script didn't set boot-file?");
	 return;
      }

      printk("Hmmm, no booting device passed as argument... ");
   } else {
      bi->default_device = chomp(bi->default_device);
      if (memcmp(bi->default_device, "--", 2) == 0) {
	 if (bi->flags & WITH_PREBOOT) {
	    printk("Preboot script didn't set boot-file?");
	    return;
	 }

         printk("Default booting device requested... ");
         prom_get_options("boot-file", bi->bootfile, sizeof(bi->bootfile));
      }
   }

   if (bi->bootfile[0] != 0) {
      printk("using boot-file '%s'\n", bi->bootfile);
      bi->default_device = bi->bootfile;
   }

   p = strchr(bi->default_device, ':');
   if (p != 0) {
      bi->default_part = strtol(p + 1, NULL, 0);
      *p++ = 0;

      /* Are we given a config file path? */
      p = strchr(p, '/');
      if (p != 0) {
         bi->config_file = p;
      } else {
         bi->config_file = "/etc/quik.conf";
      }
   }

   if (bi->default_part == 0) {

      /* No idea where's we looking for the configuration file. */
      printk("Booting device did not specify partition\n");
      return;
   }
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
   return nr;
}
