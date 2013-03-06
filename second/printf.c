/*
 * Dumb printing routines
 *
 * Copyright (C) 1996 Pete A. Zaitcev
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

#include <stdarg.h>
#include "quik.h"

/*
 * Print a string.
 */
static void printks(char *s)
{
   char c;

   while (c = *s++) {
      putchar(c);
   }
}

/*
 * Print an unsigned integer in base b, avoiding recursion.
 */
static void printkn(long n, int b)
{
   char prbuf[24];
   register char *cp;

   if (b == 10 && n < 0) {
      putchar ('-');
      n = -n;
   }

   cp = prbuf;
   do {
      *cp++ = "0123456789ABCDEF"[(int) (n % b)];
   } while (n = n / b & 0x0FFFFFFF);

   do {
      putchar (*--cp);
   } while (cp > prbuf);
}

void vprintk(char *fmt, va_list adx)
{
   register c;
   char *s;

   for (;;) {
      while ((c = *fmt++) != '%') {
         if (c == '\0') {
            putchar(0);
            return;
         }

         putchar(c);
      }
      c = *fmt++;
      if (c == 'u' || c == 'd' || c == 'o' ||
          c == 'x' || c == 'X') {
         printkn((long) va_arg(adx, unsigned),
                c == 'o' ? 8 : ((c == 'd') || (c == 'u') ? 10 : 16));
      } else if (c == 'c') {
         putchar(va_arg(adx, unsigned));
      } else if (c == 's') {
         s = va_arg(adx, char *);
         if (s) {
            printks(s);
         } else {
            printks("<NULL>");
         }
      } else if (c == 'l' || c == 'O') {
         printkn((long) va_arg(adx, long), c == 'l' ? 10 : 8);
      }
   }
}

/*
 * Scaled down version of C Library printf.
 * Only %c %s %u %d (==%u) %o %x %D %O are recognized.
 */

void printk(char *fmt,...)
{
   va_list x1;

   va_start(x1, fmt);
   vprintk(fmt, x1);
   va_end(x1);
}
