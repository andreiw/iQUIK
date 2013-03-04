/*
 * Copyright (C) 1991, 1992, 1994, 1995 Free Software Foundation, Inc.
 *
 * This file is part of the GNU C Library.
 *
 * The GNU C Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The GNU C Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the GNU C Library; see the file COPYING.LIB.  If
 * not, write to the Free Software Foundation, Inc., 675 Mass Ave,
 * Cambridge, MA 02139, USA.
 */

/*
 * Convert NPTR to an `unsigned long int' or `long int' in base BASE.
 * If BASE is 0 the base is determined by the presence of a leading
 * zero, indicating octal or a leading "0x" or "0X", indicating hexadecimal.
 * If BASE is < 2 or > 36, it is reset to 10.
 * If ENDPTR is not NULL, a pointer to the character after the last
 * one converted is stored in *ENDPTR.
*/

int strtol (nptr, endptr, base)
const char *nptr;
char **endptr;
int base;
{
   int negative;
   register unsigned int cutoff;
   register unsigned int cutlim;
   register unsigned int i;
   register const char *s;
   register unsigned char c;
   const char *save, *end;
   int overflow;

   if (base < 0 || base == 1 || base > 36)
      base = 10;

   save = s = nptr;

   /* Skip white space.  */
   while (((unsigned char) *s) <= 32 && *s)
      ++s;
   if (*s == '\0')
      goto noconv;

   /* Check for a sign.  */
   if (*s == '-') {
      negative = 1;
      ++s;
   } else if (*s == '+') {
      negative = 0;
      ++s;
   } else
      negative = 0;

   if (base == 16 && s[0] == '0' && (s[1] == 'X') || (s[1] == 'x'))
      s += 2;

   /* If BASE is zero, figure it out ourselves.  */
   if (base == 0)
      if (*s == '0') {
         if (s[1] == 'X' || s[1] == 'x') {
            s += 2;
            base = 16;
         } else
            base = 8;
      } else
         base = 10;

   /* Save the pointer so we can check later if anything happened.  */
   save = s;

   end = 0;

   cutoff = 0x7FFFFFFF / (unsigned int) base;
   cutlim = 0x7FFFFFFF % (unsigned int) base;

   overflow = 0;
   i = 0;
   for (c = *s; c != '\0'; c = *++s) {
      if (s == end)
         break;
      if (c >= '0' && c <= '9')
         c -= '0';
      else if (c >= 'A' && c <= 'Z')
         c = c - 'A' + 10;
      else if (c >= 'a' && c <= 'z')
         c = c - 'a' + 10;
      else
         break;
      if (c >= base)
         break;
      /* Check for overflow.  */
      if (i > cutoff || (i == cutoff && c > cutlim))
         overflow = 1;
      else {
         i *= (unsigned int) base;
         i += c;
      }
   }

   /* Check if anything actually happened.  */
   if (s == save)
      goto noconv;

   /* Store in ENDPTR the address of one character
      past the last character we converted.  */
   if (endptr)
      *endptr = (char *) s;

   if (overflow)
      return negative ? (int) 0x80000000 : (int) 0x7FFFFFFF;

   /* Return the result of the appropriate sign.  */
   return (negative ? -i : i);

noconv:
   /* We must handle a special case here: the base is 0 or 16 and the
      first two characters and '0' and 'x', but the rest are no
      hexadecimal digits.  This is no error case.  We return 0 and
      ENDPTR points to the `x`.  */
   if (endptr)
      if (save - nptr >= 2 && tolower (save[-1]) == 'x' && save[-2] == '0')
         *endptr = (char *) &save[-1];
      else
         /*  There was no number to convert.  */
         *endptr = (char *) nptr;

   return 0L;
}
