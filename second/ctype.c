#include <string.h>

int tolower(int c)
{
   if ('A' <= c && c <= 'Z')
      c += 'a' - 'A';
   return c;
}

int strcasecmp(const char *s1, const char *s2)
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
