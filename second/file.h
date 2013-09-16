/*
 * FS support.
 *
 * Copyright (C) 2013 Andrei Warkentin <andrey.warkentin@gmail.com>
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

#ifndef QUIK_FS_H
#define QUIK_FS_H

typedef struct {
   char *device;
   unsigned part;
   char *path;
} path_t;

quik_err_t
file_path(char *pathspec,
          char *default_device,
          unsigned default_part,
          path_t **path);

quik_err_t
file_len(path_t *path,
         length_t *len);

quik_err_t
file_load(path_t *path,
          void *buffer);

quik_err_t
file_ls(path_t *path);

#endif /* QUIK_FS_H */
