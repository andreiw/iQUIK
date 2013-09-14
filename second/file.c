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

part_t part;

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
length_file(char *device,
            int partno,
            char *filename,
            length_t *len)
{
   quik_err_t err;
   *len = 0;

   err = open_ext2(device, partno, &part);
   if (err != ERR_NONE) {
      return err;
   }

   err = ext2fs_open(filename, len);
   if (err != ERR_NONE) {
      return err;
   }

   return ERR_NONE;
}


quik_err_t
load_file(char *device,
          int partno,
          char *filename,
          void *buffer,
          void *limit,
          length_t *len)
{
   length_t size;
   quik_err_t err;

   err = open_ext2(device, partno, &part);
   if (err != ERR_NONE) {
      return err;
   }

   err = ext2fs_open(filename, &size);
   if (err != ERR_NONE) {
      return err;
   }

   if (buffer + size > limit) {
      err = ERR_FS_TOO_BIG;
      goto out;
   }

   err = ext2fs_read(buffer, size);
   if (err == ERR_NONE) {
     *len = size;
   }

out:
   return err;
}


quik_err_t
list_files(char *device,
           int partno,
           char *path)
{
   quik_err_t err;

   err = open_ext2(device, partno, &part);
   if (err != ERR_NONE) {
      return err;
   }

   return ext2fs_ls(path);
}
