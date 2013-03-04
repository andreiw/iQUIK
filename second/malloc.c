/*
 * Dumb memory allocation routines
 *
 * Copyright (C) 1997 Paul Mackerras
 * 1996 Maurizio Plaza
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

#include <layout.h>

static char *malloc_ptr = (char *) MALLOC_BASE;
static char *last_alloc = 0;

void *malloc (unsigned int size)
{
   char *caddr;

   *(int *)malloc_ptr = size;
   caddr = malloc_ptr + sizeof(int);
   malloc_ptr += size + sizeof(int);
   last_alloc = caddr;
   malloc_ptr = (char *) ((((unsigned int) malloc_ptr) + 3) & (~3));
   return caddr;
}

void *realloc(void *ptr, unsigned int size)
{
   char *caddr, *oaddr = ptr;

   if (oaddr == last_alloc) {
      *(int *)(oaddr - sizeof(int)) = size;
      malloc_ptr = oaddr + size;
      return oaddr;
   }
   caddr = malloc(size);
   if (caddr != 0 && oaddr != 0)
      memcpy(caddr, oaddr, *(int *)(oaddr - sizeof(int)));
   return caddr;
}

void free (void *m)
{
   if (m == last_alloc)
      malloc_ptr = (char *) last_alloc - sizeof(int);
}

void mark (void **ptr)
{
   *ptr = (void *) malloc_ptr;
}

void release (void *ptr)
{
   malloc_ptr = (char *) ptr;
}

void *strdup(char *str)
{
   char *p = malloc(strlen(str) + 1);

   strcpy(p, str);
   return p;
}
