/*
 * Command handling.
 *
 * Copyright (C) 2013-2014 Andrei Warkentin <andrey.warkentin@gmail.com>
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
#include "commands.h"

#define CMD_LENG 512
static char *cbuff;


static inline void
cmd_show_command(command_t *c)
{
   table_print_t tp;

   table_print_start(&tp, 2, 10);
   table_print(&tp, c->name);
   printk("- ");
   table_print(&tp, c->desc);
   table_print_end(&tp);
}


static quik_err_t
cmd_help(char *args)
{
   char *rem;
   unsigned add = 0;
   command_t *c = &_commands;

   word_split(&args, &rem);
   if (args != NULL &&
       args[0] != '!') {
      add = 1;
   }

   while (c < &_commands_end) {
      if (args == NULL ||
          !strcmp(args, c->name + add)) {
         cmd_show_command(c);
         if (args != NULL) {
            break;
         }
      }

      c++;
   }

   if (c == &_commands_end) {
      cmd_show_commands();
   }

   return ERR_NONE;
}

COMMAND(help, cmd_help, "show help, given an optional command name");


void
cmd_show_commands(void)
{
  command_t *c = &_commands;
  table_print_t tp;

  table_print_start(&tp, 4, 10);

  printk("Available commands:\n");
  while (c < &_commands_end) {
     table_print(&tp, c->name);
     c++;
  }

  table_print_end(&tp);
}


static quik_err_t
cmd_dispatch(char *args)
{
   command_t *c = &_commands;
   quik_err_t err;
   char *e;

   word_split(&args, &e);
   if (args == NULL) {
      return ERR_CMD_UNKNOWN;
   }

   while (c < &_commands_end) {
      if (!strcmp(args, c->name)) {
         break;
      }

      c++;
   }

   if (c == &_commands_end) {
      return ERR_CMD_UNKNOWN;
   }

   e = chomp(e);

   err = c->fn(e);
   if (err != ERR_NONE) {
      if (err == ERR_CMD_BAD_PARAM) {
         cmd_show_command(c);
      } else {
         printk("'%s': %r\n", c->name, err);
      }
   }

   return ERR_NONE;
}

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
         cbuff[x] = 0;

         /*
          * Should allow completion.
          */
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

   putchar('\n');
   cbuff[x] = 0;
   if (cbuff[0] == '!') {
      if (cmd_dispatch(cbuff) != ERR_NONE) {
         cmd_show_commands();
      }

      return NULL;
   }

   return cbuff;
}
