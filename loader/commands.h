/*
 * Command handling.
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

#ifndef QUIK_COMMANDS_H
#define QUIK_COMMANDS_H

#define CMD_STRCMP
#define CMD_MEMCMP

typedef struct {
   char *name;
   quik_err_t (*fn)(char *args);
   char *desc;
} command_t;

#define COMMAND(n, f, d)                                            \
   command_t cmd_desc_ ## n __attribute__ ((section (".commands"))) = { .name = "!"#n, .fn = f, .desc = d }

extern command_t _commands;
extern command_t _commands_end;

quik_err_t cmd_init(void);
char *cmd_edit(void (*tabfunc)(char *buf), key_t c);
void cmd_show_commands(void);

#endif /* QUIK_COMMANDS_H */
