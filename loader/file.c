/*
 * File related stuff.
 *
 * Copyright (C) 2013 Andrei Warkentin <andrey.warkentin@gmail.com>
 * Copyright (C) 1996 Maurizio Plaza
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
#include "ext2fs.h"
#include "commands.h"

static part_t part;

static quik_err_t
open_ext2(char *device,
          int partno,
          part_t *part)
{
   quik_err_t err;
   int old_was_mounted;

   old_was_mounted = part->flags & PART_MOUNTED;

   err = part_open(device, partno, part);
   if (err != ERR_NONE) {
      return err;
   }

   if (part->flags & PART_MOUNTED) {
      return ERR_NONE;
   }

   if (old_was_mounted) {
      ext2fs_close();
   }

   err = ext2fs_mount(part);
   if (err != ERR_NONE) {
      return err;
   }

   return ERR_NONE;
}


quik_err_t
file_len(path_t *path,
         length_t *len)
{
   quik_err_t err;
   *len = 0;

   err = open_ext2(path->device, path->part, &part);
   if (err != ERR_NONE) {
      return err;
   }

   err = ext2fs_open(path->path, len);
   if (err != ERR_NONE) {
      return err;
   }

   return ERR_NONE;
}


quik_err_t
file_load(path_t *path,
          void *buffer)
{
   length_t size;
   quik_err_t err;

   err = open_ext2(path->device, path->part, &part);
   if (err != ERR_NONE) {
      return err;
   }

   err = ext2fs_open(path->path, &size);
   if (err != ERR_NONE) {
      return err;
   }

   return ext2fs_read(buffer, size);
}


quik_err_t
file_ls(path_t *path)
{
   quik_err_t err;

   err = open_ext2(path->device, path->part, &part);
   if (err != ERR_NONE) {
      return err;
   }

   return ext2fs_ls(path->path);
}


quik_err_t
file_path(char *pathspec,
          env_dev_t *default_dev,
          path_t **path)
{
   int n;
   char *endp;
   quik_err_t err;
   unsigned slen = strlen(pathspec) + 1;
   path_t *p;

   p = (path_t *) malloc(slen + sizeof(path_t));
   if (p == NULL) {
      return ERR_NO_MEM;
   }

   memcpy((void *) (p + 1), pathspec, slen);
   pathspec = (char *) (p + 1);
   pathspec = chomp(pathspec);

   p->path = strchr(pathspec, ':');
   if (!p->path) {
      err = env_dev_is_valid(default_dev);
      if (err != ERR_NONE) {
         return err;
      }

      /*
       * Short form using default device/part.
       */
      p->path = pathspec;
      p->part = default_dev->part;
      p->device = default_dev->device;
   } else {

      /*
       * Colon presence means the path is treated as 'device:part/fspath'
       * or as 'device:part'. Thus if fspath contains colons (unlikely),
       * the full path must be given.
       */
      p->path[0] = '\0';
      p->path++;
      p->device = pathspec;

      if (strlen(p->device) == 0) {

         /*
          * Nothing before the colon...?
          */
         goto bad_path;
      }

      n = strtol(p->path, &endp, 0);
      if (endp == p->path) {

         /*
          * We had a colon, but it wasn't followed by the
          * partition number, bad path.
          */
         goto bad_path;
      }

      p->part = n;
      p->path = endp;
   }

   /* Path */
   if (p->path[0] != '/') {
      if (p->path[0] == '\0') {
         p->path = "/";
      } else {
         goto bad_path;
      }
   }

   *path = p;
   return ERR_NONE;

bad_path:
   free(p);
   return ERR_FS_PATH;
}


quik_err_t
file_cmd_dev(char *p)
{
   quik_err_t err;
   path_t *path = NULL;

   /* Need a path. */
   if (strlen(p) == 0) {
      return ERR_CMD_BAD_PARAM;
   }

   err = file_path(p, &bi->default_dev, &path);
   if (err != ERR_NONE) {
      return err;
   }

   /*
    * XXX: we end up leaking path. The real
    * fix is to introduce env_dev_set and
    * always free/strdup on changes.
    */
   bi->default_dev.device = path->device;
   env_dev_set_part(&bi->default_dev, path->part);

   return env_dev_is_valid(&bi->default_dev);
}

COMMAND(dev, file_cmd_dev, "change default device path given a device:part");


quik_err_t
file_cmd_cat(char *p)
{
   char *message;
   length_t len;
   quik_err_t err;
   path_t *path = NULL;

   /* Need a path. */
   if (strlen(p) == 0) {
      return ERR_CMD_BAD_PARAM;
   }

   err = file_path(p, &bi->default_dev, &path);
   if (err != ERR_NONE) {
      if (err == ERR_ENV_CURRENT_BAD) {
         err = ERR_ENV_DEFAULT_BAD;
      }

      return err;
   }

   err = file_len(path, &len);
   if (err != ERR_NONE) {
      free(path);
      return err;
   }

   message = malloc(len);
   if (message == NULL) {
      free(path);
      return err;
   }
   err = file_load(path, message);
   if (err == ERR_NONE) {
      message[len] = 0;
      printk("%s", message);
   }

   free(message);
   free(path);
   return err;
}

COMMAND(cat, file_cmd_cat, "show file contents given a [device:part]/fs/path");


static quik_err_t
file_cmd_ls(char *args)
{
   path_t *path;
   quik_err_t err;

   /*
    * Nothing means just list the current device.
    */
   if (strlen(args) == 0) {
      args = "/";
   }

   err = file_path(args, &bi->default_dev, &path);
   if (err != ERR_NONE) {
      if (err == ERR_ENV_CURRENT_BAD) {
         err = ERR_ENV_DEFAULT_BAD;
      }

      return err;
   }

   printk("Listing '%P'\n", path);
   err = file_ls(path);

   free(path);
   return err;
}

COMMAND(ls, file_cmd_ls, "list files given a [device:part][/fs/path]");

