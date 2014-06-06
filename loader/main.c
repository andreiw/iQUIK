/*
 * Second stage boot loader
 *
 * Copyright (C) 2013 Andrei Warkentin <andrey.warkentin@gmail.com>
 * Copyright (C) 1996 Paul Mackerras.
 *
 * Because this program is derived from the corresponding file in the
 * silo-0.64 distribution, it is also
 *
 * Copyright (C) 1996 Pete A. Zaitcev
 * 1996 Maurizio Plaza
 * 1996 David S. Miller
 * 1996 Miguel de Icaza
 * 1996 Jakub Jelinek
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
#include "file.h"
#include "prom.h"
#include <layout.h>

#define PROMPT "boot: "

boot_info_t *bi = { 0 };


static void
maintabfunc(char *buf)
{
   if (bi->flags & CONFIG_VALID) {
      cfg_print_images();
      printk(PROMPT"%s", buf);
   }
}


static char *
make_params(char *label,
            char *params)
{
   char *p, *q;
   static char *buffer = NULL;

   /*
    * AndreiW: fix me. This is mildly
    * better than putting it on the heap.
    */
   if (buffer == NULL) {
      buffer = malloc(2048);
      if (buffer == NULL) {
         return NULL;
      }
   }

   q = buffer;
   *q = 0;

   p = cfg_get_strg(label, "literal");
   if (p) {
      strcpy(q, p);
      q = strchr(q, 0);
      if (*params) {
         if (*p)
            *q++ = ' ';
         strcpy(q, params);
      }
      return buffer;
   }

   p = cfg_get_strg(label, "root");
   if (p) {
      strcpy (q, "root=");
      strcpy (q + 5, p);
      q = strchr (q, 0);
      *q++ = ' ';
   }
   if (cfg_get_flag(label, "read-only")) {
      strcpy (q, "ro ");
      q += 3;
   }
   if (cfg_get_flag(label, "read-write")) {
      strcpy (q, "rw ");
      q += 3;
   }
   p = cfg_get_strg(label, "ramdisk");
   if (p) {
      strcpy (q, "ramdisk=");
      strcpy (q + 8, p);
      q = strchr (q, 0);
      *q++ = ' ';
   }
   p = cfg_get_strg (label, "append");
   if (p) {
      strcpy (q, p);
      q = strchr (q, 0);
      *q++ = ' ';
   }
   *q = 0;

   if (cfg_get_flag (label, "pause-after")) {
      bi->flags |= PAUSE_BEFORE_BOOT;
   }

   p = cfg_get_strg(label, "pause-message");
   if (p) {
      bi->pause_message = p;
   }

   if (*params) {
      strcpy(q, params);
   }

   return buffer;
}


static void
command_ls(char *args)
{
   path_t *path;
   quik_err_t err;

   /*
    * Nothing means just list the current device.
    */
   args = chomp(args);
   if (strlen(args) == 0) {
      args = "/";
   }

   err = file_path(args, &bi->default_dev, &path);
   if (err != ERR_NONE) {
      if (err == ERR_ENV_CURRENT_BAD) {
         err = ERR_ENV_DEFAULT_BAD;
      }

      printk("Error listing '%s': %r\n",
             args, err);
      return;
   }

   printk("Listing '%P'\n", path);
   err = file_ls(path);
   if (err != ERR_NONE) {
      printk("Error listing '%P': %r\n",
             path, err);
   }

   free(path);
}


static void
command_cat(char *p)
{
   char *message;
   length_t len;
   quik_err_t err;
   path_t *path = NULL;

   /*
    * Nothing means nothing.
    */
   p = chomp(p);
   if (strlen(p) == 0) {
      printk("No file path provided\n");
      return;
   }

   err = file_path(p, &bi->default_dev, &path);
   if (err != ERR_NONE) {
      if (err == ERR_ENV_CURRENT_BAD) {
         err = ERR_ENV_DEFAULT_BAD;
      }

      printk("Error opening '%s': %r\n",
             p, err);
      return;
   }

   err = file_len(path, &len);
   if (err != ERR_NONE) {
      printk("Error opening '%P': %r\n",
             path, err);
      free(path);
      return;
   }

   message = malloc(len);
   if (message == NULL) {
      printk("Error catting '%P': %r\n",
             path, ERR_NO_MEM);
      free(path);
      return;
   }
   err = file_load(path, message);
   if (err == ERR_NONE) {
      message[len] = 0;
      printk("%s", message);
   } else {
      printk("Error opening '%P': %r\n",
             path, err);
   }

   free(message);
   free(path);
}


static quik_err_t
load_config(void)
{
   length_t len;
   char *buf;
   char *endp;
   char *p;
   path_t path;
   unsigned n = 0;
   quik_err_t err = ERR_NONE;
   char *attempts[] = {
      bi->config_file,
      "/boot/iquik.conf",
      "/boot/quik.conf",
      "/boot/iquik.conf",
      "/quik.conf",
      NULL
   };

   err = env_dev_is_valid(&bi->default_dev);
   if (err != ERR_NONE) {
      if (err == ERR_ENV_CURRENT_BAD) {
         err = ERR_ENV_DEFAULT_BAD;
      }

      return err;
   }

   path.device = bi->default_dev.device;
   path.part = bi->default_dev.part;
   while (attempts[n] != NULL) {
      path.path = attempts[n];

      printk("Trying configuration file @ '%P'\n", &path);
      err = file_len(&path, &len);
      if (err == ERR_NONE) {
         break;
      }

      n++;
   }

   /* Set by file_len. */
   if (err != ERR_NONE) {
      return ERR_CONFIG_NOT_FOUND;
   }

   buf = malloc(len);
   if (buf == NULL) {
      return ERR_NO_MEM;
   }

   err = file_load(&path, buf);
   if (err != ERR_NONE) {
      printk("\nCouldn't load '%P': %r\n", &path, err);
      free(buf);
      return err;
   }

   if (cfg_parse(bi->config_file, buf, len) < 0) {
      printk ("Syntax error or read error in '%P'\n", &path);
   }

   free(buf);
   bi->flags |= CONFIG_VALID;
   p = cfg_get_strg(0, "init-code");
   if (p) {
      prom_interpret(p);
   }

   p = cfg_get_strg(0, "init-message");
   if (p) {
      printk("%s\n", p);
   }

   if(cfg_get_strg(0, "device") != NULL) {
      bi->default_dev.device = cfg_get_strg(0, "device");
   }

   p = cfg_get_strg(0, "partition");
   if (p) {
      n = strtol(p, &endp, 10);
      if (endp != p && *endp == 0) {
         env_dev_set_part(&bi->default_dev, n);
      }
   }

   p = cfg_get_strg(0, "pause-message");
   if (p) {
      bi->pause_message = p;
   }

   p = cfg_get_strg(0, "message");
   if (p) {
      command_cat(p);
   }

   return ERR_NONE;
}


static void
bang_commands(char *args)
{
   if (!strcmp(args, "debug")) {
      bi->flags |= DEBUG_BEFORE_BOOT;
   } else if (!strcmp(args, "shim")) {
      bi->flags |= SHIM_OF;
   } else if (!strcmp(args, "halt")) {
      prom_pause(NULL);
   } else if (!memcmp(args, "ls", 2)) {
      command_ls(args + 2);
   } else if (!memcmp(args, "cat ", 4)) {
      command_cat(args + 3);
   } else if (!memcmp(args, "of ", 3)) {
      prom_interpret(args + 3);
      printk("\n");
   } else {
      printk("Available commands: !debug, !shim, !halt, !ls, !cat, !of\n");
   }
}


static quik_err_t
get_params(char **kernel,
           char **initrd,
           char **params,
           env_dev_t *cur_dev)
{
   char *p;
   char *q;
   int n;
   char *buf;
   char *endp;
   key_t lastkey;
   char *label = NULL;
   int timeout = DEFAULT_TIMEOUT;

   if ((bi->flags & TRIED_AUTO) == 0) {
      bi->flags ^= TRIED_AUTO;
      *params = bi->bootargs;
      *kernel = *params;

      word_split(kernel, params);
      if (!*kernel) {
         *kernel = cfg_get_default();

         /*
          * Timeout only makes sense
          * if we don't have an image name already
          * passed.
          */
         if ((bi->flags & CONFIG_VALID) &&
             (q = cfg_get_strg(0, "timeout")) != 0 && *q != 0) {
           timeout = strtol(q, NULL, 0);
         }
      } else {

         /*
          * We have something, boot immediately.
          */
         timeout = 0;
      }
   }

   printk(PROMPT);
   lastkey = KEY_NONE;
   if (timeout != -1) {
      lastkey = wait_for_key(timeout, '\n');
   }

   if (lastkey == '\n') {
      printk("%s", *kernel);
      if (*params) {
         printk(" %s", *params);
      }

      printk("\n");
   } else {
      *kernel = NULL;

      buf = cmd_edit(maintabfunc, lastkey);
      printk("\n");

      if (buf[0] == '!') {
         bang_commands(buf + 1);
         return ERR_NOT_READY;
      }

      *kernel = buf;
      word_split(kernel, params);
   }

   if (bi->flags & CONFIG_VALID) {
      *initrd = cfg_get_strg(0, "initrd");
      p = cfg_get_strg(*kernel, "image");
      if (p && *p) {
         label = *kernel;
         *kernel = p;

         p = cfg_get_strg(label, "device");
         if (p) {
            cur_dev->device = p;
         }

         p = cfg_get_strg(label, "partition");
         if (p) {
            n = strtol(p, &endp, 10);
            if (endp != p && *endp == 0) {
               env_dev_set_part(cur_dev, n);
            }
         }

         p = cfg_get_strg(label, "initrd");
         if (p) {
            *initrd = p;
         }

         if (cfg_get_strg(label, "old-kernel")) {
            bi->flags |= BOOT_PRE_2_4;
         } else {
            bi->flags &= ~BOOT_PRE_2_4;
         }

         *params = make_params(label, *params);
         if (*params == NULL) {
            return ERR_NO_MEM;
         }
      }
   }

   if (*kernel == NULL) {
      printk("<TAB> for list of bootable images, or !help\n");
      return ERR_NOT_READY;
   }

   /*
    * If we manually entered kernel path, the initrd could
    * be following in the param list...
    */
   if (label == NULL) {
      *params = chomp(*params);

      if (memcmp(*params, "-- ", 3) != 0) {
         *initrd = *params;
         word_split(initrd, params);
      } else {

         /*
          * No initrd, just kernel args.
          */
         *params = *params + 3;
      }
   }

   return ERR_NONE;
}


static quik_err_t
get_load_paths(path_t **kernel,
               path_t **initrd,
               char **params)
{
   quik_err_t err;
   char *kernel_spec = NULL;
   char *initrd_spec = NULL;
   env_dev_t cur_dev = { 0 };

   err = get_params(&kernel_spec, &initrd_spec,
                    params, &cur_dev);
   if (err != ERR_NONE) {
      return err;
   }

   /*
    * In case get_params didn't set the device or partition,
    * propagate from the default device path.
    */
   err = env_dev_update_from_default(&cur_dev);
   if (err != ERR_NONE) {
      return err;
   }

   err = file_path(kernel_spec,
                   &cur_dev,
                   kernel);
   if (err != ERR_NONE) {
      printk("Error parsing kernel path '%s': %r\n", kernel_spec, err);
      return err;
   }

   if (initrd_spec != NULL) {
      err = file_path(initrd_spec,
                      &cur_dev,
                      initrd);
      if (err != ERR_NONE) {
         printk("Error parsing initrd path '%s': %r\n", initrd_spec, err);
         free(kernel);
         return err;
      }
   }

   return ERR_NONE;
}


static quik_err_t
load_image(path_t *path,
           vaddr_t *where,
           length_t *len)
{
   quik_err_t err;

   printk("Loading '%P'\n", path);
   err = file_len(path, len);
   if (err != ERR_NONE) {
      printk("Error fetching size for '%P': %r\n",
             path, err);
      return err;
   }

   *where = (vaddr_t) prom_claim_chunk((void *) *where, *len);
   if (*where == (vaddr_t) -1) {
      printk("Couldn't claim 0x%x bytes to load '%P'\n", *len, path);
      return ERR_NO_MEM;
   }

   err = file_load(path, (void *) *where);
   if (err != ERR_NONE) {
      printk("Error loading '%P': %r\n", path, err);
      prom_release((void *) *where, *len);
      return err;
   }

   return ERR_NONE;
}


static quik_err_t
try_load_loop(load_state_t *image,
              char **params)
{
   quik_err_t err;
   vaddr_t kernel_buf = 0;
   length_t kernel_len;
   vaddr_t initrd_buf = 0;
   length_t initrd_len;
   path_t *kernel_path = NULL;
   path_t *initrd_path = NULL;

   err = get_load_paths(&kernel_path, &initrd_path, params);
   if (err != ERR_NONE) {
      return err;
   }

   kernel_buf = LOAD_BASE;
   err = load_image(kernel_path, &kernel_buf, &kernel_len);
   if (err == ERR_NONE) {
      err = elf_parse((void *) kernel_buf, kernel_len, image);
   }

   if (err != ERR_NONE) {
      printk("Error loading '%P': %r\n", kernel_path, err);
      goto out;
   }

   if (!initrd_path) {
      bi->initrd_base = 0;
      bi->initrd_len = 0;
      return ERR_NONE;
   }

   initrd_buf = kernel_buf + kernel_len;
   err = load_image(initrd_path, &initrd_buf, &initrd_len);
   if (err != ERR_NONE) {
      goto out;
   }

   bi->initrd_base = initrd_buf;
   bi->initrd_len = initrd_len;
   return ERR_NONE;

out:
   if (kernel_buf) {
      prom_release((void *) kernel_buf, kernel_len);
   }

   if (initrd_path) {
      free(initrd_path);
   }

   if (kernel_path) {
      free(kernel_path);
   }

   return err;
}


/* Here we are launched */
void
iquik_main(void *a1,
           void *a2,
           void *prom_entry)
{
   char *params;
   quik_err_t err;
   load_state_t image;

   err = prom_init(prom_entry);
   if (err != ERR_NONE) {
      prom_exit();
   }

   printk("\niQUIK OldWorld Bootloader\n");
   printk("Copyright (C) 2014 Andrei Warkentin <andrey.warkentin@gmail.com>\n");
   if (bi->flags & SHIM_OF) {
      printk("This firmware requires a shim to work around bugs\n");
   }

   err = malloc_init();
   if (err != ERR_NONE) {
      goto error;
   }

   err = cmd_init();
   if (err != ERR_NONE) {
      goto error;
   }

   err = env_init();
   if (err != ERR_NONE) {
      goto error;
   }

   err = load_config();
   if (err != ERR_NONE) {
      printk("No configration file parsed: %r\n", err);
   }

   for (;;) {
      params = NULL;

      err = try_load_loop(&image, &params);
      if (err == ERR_NONE) {
         break;
      }
   }

   err = elf_relo(&image);
   if (err != ERR_NONE) {
      goto error;
   }

   if (bi->flags & BOOT_PRE_2_4 &&
       bi->flags & SHIM_OF) {
      printk("OF shimming unsupported for pre-2.4 kernels\n");
      bi->flags ^= SHIM_OF;
   }

   if (bi->flags & DEBUG_BEFORE_BOOT) {
      if (bi->flags & BOOT_PRE_2_4) {
         printk("Booting pre-2.4 kernel\n");
      }

      printk("Kernel: 0x%x @ 0x%x\n", image.text_len, image.linked_base);
      printk("Initrd: 0x%x @ 0x%x\n", bi->initrd_len, bi->initrd_base);
      printk("Kernel parameters: %s\n", params);
      printk("Kernel entry: 0x%x\n", image.entry);
      prom_pause(NULL);
   } else if (bi->flags & PAUSE_BEFORE_BOOT) {
      prom_pause(bi->pause_message);
   }

   err = elf_boot(&image, params);
error:
   printk("Exiting on error: %r", err);
   prom_exit();
}
