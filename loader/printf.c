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

#include "quik.h"
#include "prom.h"
#include "file.h"

#ifndef CONFIG_TINY
#define QUIK_ERR_DEF(e, s) s,
static char *errors[] = {
   QUIK_ERR_LIST
};
#undef QUIK_ERR_DEF
#endif /* CONFIG_TINY */

/*
 * Print a string.
 */
static void printks(char *s)
{
   char c;
   char *null_string = "<NULL>";

   if (s == NULL) {
      s = null_string;
   }

   while ((c = *s++) != '\0') {
      putchar(c);
   }
}


#ifndef CONFIG_TINY
/*
 * Print an error.
 */
static void printkr(quik_err_t err)
{
   if (err < ERR_INVALID) {
      printks(errors[err]);
   } else {
      printks(errors[ERR_INVALID]);
   }
}
#endif /* CONFIG_TINY */


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
   } while ((n = n / b & 0x0FFFFFFF) != 0);

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
   } while ((n = n / b & 0x0FFFFFFF) != 0);

   do {
      putchar (*--cp);
   } while (cp > prbuf);
}


/*
 * Print a path.
 */
static void printkp(path_t *path)
{
   if (path == NULL) {
      printks(NULL);
   }

   printks(path->device);
   putchar(':');
   printknu(path->part, 10);
   printks(path->path);
}


void vprintk(char *fmt, va_list adx)
{
   quik_err_t err;
   path_t *path;
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
         printkns((long) va_arg(adx, long), 10);
      } else if (c == 'c') {
         putchar(va_arg(adx, unsigned));
      } else if (c == 's') {
         s = va_arg(adx, char *);
         printks(s);
      } else if (c == 'r') {
         err = va_arg(adx, quik_err_t);
#ifndef CONFIG_TINY
         printkr(err);
#else
         printknu(err, 16);
#endif /* CONFIG_TINY */
      } else if (c == 'P') {
         path = va_arg(adx, path_t *);
         printkp(path);
      } else if (c == 'p') {
         s = va_arg(adx, void *);
         if (s) {
            putchar('0');
            putchar('x');
            printknu((long) s, 16);
         } else {
            printks(NULL);
         }
      }
   }
}

/*
 * Scaled down version of C Library printf.
 * Only %c %s %u %d %i %u %o %x %p %r %P are recognized.
 */

void printk(char *fmt,...)
{
   va_list x1;

   va_start(x1, fmt);
   vprintk(fmt, x1);
   va_end(x1);
}
