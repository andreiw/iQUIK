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
static void printknu(unsigned n, int b)
{
   char prbuf[24];
   register char *cp;

   cp = prbuf;
   do {
      *cp++ = "0123456789ABCDEF"[(int) (n % b)];
   } while (n = n / b & 0x0FFFFFFF);

   do {
      putchar (*--cp);
   } while (cp > prbuf);
}


/*
 * Print an signed integer in base b, avoiding recursion.
 */
static void printkns(long n, int b)
{
   char prbuf[24];
   register char *cp;

   if (n < 0) {
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
   char *n = "<NULL>";
   char *s;
   char c;

   for (;;) {
      while ((c = *fmt++) != '%') {
         if (c == '\0') {
            putchar(0);
            return;
         }

         putchar(c);
      }

      c = *fmt++;
      if (c == 'u' || c == 'o' ||
          c == 'x' || c == 'X') {
         printknu((unsigned) va_arg(adx, unsigned),
                  c == 'o' ? 8 : (c == 'u' ? 10 : 16));
      } else if (c == 'i' || c == 'd')  {
         printknu((long) va_arg(adx, long), 10);
      } else if (c == 'c') {
         putchar(va_arg(adx, unsigned));
      } else if (c == 's') {
         s = va_arg(adx, char *);
         if (s) {
            printks(s);
         } else {
            printks(n);
         }
      } else if (c == 'p') {
         s = va_arg(adx, void *);
         if (s) {
            putchar('0');
            putchar('x');
            printknu((long) s, 16);
         } else {
            printks(n);
         }
      }
   }
}

/*
 * Scaled down version of C Library printf.
 * Only %c %s %u %d %i %u %o %x %p  are recognized.
 */

void printk(char *fmt,...)
{
   va_list x1;

   va_start(x1, fmt);
   vprintk(fmt, x1);
   va_end(x1);
}
