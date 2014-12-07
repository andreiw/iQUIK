/*
 * Boot environment.
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
#include "prom.h"

#define ENV_SZ 512

void
env_dev_set_part(env_dev_t *dp, unsigned part)
{
   dp->part = part;
   dp->part_valid = true;
}

quik_err_t
env_dev_is_valid(env_dev_t *dp)
{
   if (dp->device == NULL ||
       !dp->part_valid) {
      return ERR_ENV_CURRENT_BAD;
   }

   return ERR_NONE;
}

quik_err_t
env_dev_update_from_default(env_dev_t *cur_dev)
{
   if (env_dev_is_valid(&bi->default_dev) != ERR_NONE) {
      return ERR_ENV_DEFAULT_BAD;
   }

   *cur_dev = bi->default_dev;

   return ERR_NONE;
}

quik_err_t
env_init(void)
{
   char *p;

   bi->of_bootargs_buf = malloc(ENV_SZ);
   bi->of_bootfile_buf = malloc(ENV_SZ);
   if (bi->of_bootargs_buf == NULL ||
       bi->of_bootfile_buf == NULL) {
      return ERR_NO_MEM;
   }

   prom_get_chosen("bootargs", bi->of_bootargs_buf, ENV_SZ);
   prom_get_options("boot-file", bi->of_bootfile_buf, ENV_SZ);

#ifndef CONFIG_TINY
   printk("/chosen/bootargs = '%s'\n", bi->of_bootargs_buf);
   printk("       boot-file = '%s'\n", bi->of_bootfile_buf);
#endif /* CONFIG_TINY */

   if (bi->flags & WITH_PREBOOT) {

      /*
       * Forget about the passed args, presumably boot-file was set
       * by the script (why else would you use it)?
       *
       * Keep in mind that if booting via boot-device, the boot-file gets stuffed by OF
       * in boot-args, so, we can't just use bootargs to override the preboot script,
       * and we have to ignore them.
       */
      bi->default_dev.device = bi->of_bootfile_buf;
   } else {
      bi->default_dev.device = bi->of_bootargs_buf;
   }

   word_split(&bi->default_dev.device, &bi->bootargs);

   if (!bi->default_dev.device) {
      if (bi->flags & WITH_PREBOOT) {

         /*
          * For preboot we already basically looked at boot-file, so
          * don't bother looking at it again.
          */
         return ERR_ENV_PREBOOT_BAD;
      }

      /*
       * No booting device passed via /chosen/bootargs, try
       * bi->of_bootfile_buf.
       */
      bi->default_dev.device = chomp(bi->of_bootfile_buf);
   } else {
      if (memcmp(bi->default_dev.device, "--", 2) == 0) {
         if (bi->flags & WITH_PREBOOT) {

            /*
             * Preboot wanted us to use default, but uh... it overrode the default.
             */
            return ERR_ENV_PREBOOT_BAD;
         }

         /*
          * Use the default from NVRAM in bi->of_bootfile_buf.
          */
         bi->default_dev.device = chomp(bi->of_bootfile_buf);
      }
   }

   p = strchr(bi->default_dev.device, ':');
   if (p != 0) {
      env_dev_set_part(&bi->default_dev, strtol(p + 1, NULL, 0));
      *p++ = 0;

      /* Are we given a config file path? */
      p = strchr(p, '/');
      if (p != 0) {
         bi->config_file = p;
      } else {
         bi->config_file = "/etc/quik.conf";
      }
   }

   return ERR_NONE;
}
