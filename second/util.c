/*
 * Miscellaneous libc-type stuff.
 *
 * Copyright (C) 2013 Andrei Warkentin <andrey.warkentin@gmail.com>
 * Copyright (C) 1996 Paul Mackerras.
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

int
tolower(int c)
{
   if ('A' <= c && c <= 'Z')
      c += 'a' - 'A';
   return c;
}


void *
strdup(char *str)
{
   char *p = malloc(strlen(str) + 1);

   strcpy(p, str);
   return p;
}


uint16_t
swab16(uint16_t value)
{
   uint16_t result;

   __asm__("rlwimi %0,%1,8,16,23\n\t"
           : "=r" (result)
           : "r" (value), "0" (value >> 8));
   return result;
}


uint32_t
swab32(uint32_t value)
{
   uint32_t result;

   __asm__("rlwimi %0,%1,24,16,23\n\t"
           "rlwimi %0,%1,8,8,15\n\t"
           "rlwimi %0,%1,24,0,7"
           : "=r" (result)
           : "r" (value), "0" (value >> 24));
   return result;
}


int
strcasecmp(const char *s1, const char *s2)
{
   int c1, c2;

   for (;;) {
      c1 = *s1++;
      if ('A' <= c1 && c1 <= 'Z')
         c1 += 'a' - 'A';
      c2 = *s2++;
      if ('A' <= c2 && c2 <= 'Z')
         c2 += 'a' - 'A';
      if (c1 != c2 || c1 == 0)
         return c1 - c2;
   }
}


char *
strstr(const char * s1,
       const char * s2)
{
   int l1, l2;

   l2 = strlen(s2);
   if (!l2)
      return (char *) s1;
   l1 = strlen(s1);
   while (l1 >= l2) {
      l1--;
      if (!memcmp(s1,s2,l2))
         return (char *) s1;
      s1++;
   }
   return NULL;
}


void
spinner(int freq)
{
   static int i = 0;
   static char rot[] = "\\|/-";

   if (!(i % freq)) {
      printk ("%c\b", rot[(i / freq) % 4]);
   }

   i++;
}
