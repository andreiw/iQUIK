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
          char *default_device,
          unsigned default_part,
          path_t **path)
{
   int n;
   char *endp;
   unsigned slen = strlen(pathspec) + 1;
   path_t *p = (path_t *) malloc(slen + sizeof(path_t));

   if (p == NULL) {
      return ERR_NO_MEM;
   }

   memcpy((void *) (p + 1), pathspec, slen);
   pathspec = (char *) (p + 1);
   pathspec = chomp(pathspec);

   p->path = strchr(pathspec, ':');
   if (!p->path) {
      p->path = pathspec;
      p->device = default_device;
   } else {
      p->path[0] = '\0';
      p->path++;
      p->device = pathspec;
   }

   n = strtol(p->path, &endp, 0);
   if (endp != p->path) {
      p->part = n;
      p->path = endp;
   } else {
      p->part = default_part;
   }

   /* Path */
   if (p->path[0] != '/') {
      free(p);
      return ERR_FS_PATH;
   }

   *path = p;
   return ERR_NONE;
}
