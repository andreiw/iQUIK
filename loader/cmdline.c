/*
 * Prompt handling
 *
 * Copyright (C) 2014 Andrei Warkentin
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
#include "prom.h"

#define CMD_LENG 512
static char *cbuff;

quik_err_t
cmd_init(void)
{
   cbuff = malloc(CMD_LENG);
   if (cbuff == NULL) {
      return ERR_NO_MEM;
   }

   return ERR_NONE;
}

char *
cmd_edit(void (*tabfunc)(char *), key_t c)
{
   int x = 0;
   memset(cbuff, 0, sizeof(cbuff));

   if (c == KEY_NONE) {
      c = getchar();
   }
   while (c != KEY_NONE && c != '\n' && c != '\r') {
      if (c == '\t' && tabfunc) {
        (*tabfunc)(cbuff);
      } else if (c == '\b' || c == 0x7F) {
         if (x > 0) {
            --x;
            cbuff[x] = 0;
            prom_print("\b \b");
         }
      } else if (c >= ' ' && x < CMD_LENG - 1) {
         cbuff[x] = c;
         putchar(c);
         ++x;
      }
      c = getchar();
   }

   cbuff[x] = 0;
   return cbuff;
}
