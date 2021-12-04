/*
 * Handling and parsing of quik.conf
 *
 * Copyright (C) 1995 Werner Almesberger
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
#include "isetjmp.h"
#include "prom.h"

typedef enum {
   cft_strg, cft_flag, cft_end
} CONFIG_TYPE;

typedef struct {
   CONFIG_TYPE type;
   char *name;
   void *data;
} CONFIG;

#define MAX_TOKEN 200
#define MAX_VAR_NAME MAX_TOKEN
#define EOF -1

CONFIG cf_options[] =
{
   {cft_strg, "device", NULL},
   {cft_strg, "partition", NULL},
   {cft_strg, "default", NULL},
   {cft_strg, "timeout", NULL},
   {cft_strg, "message", NULL},
   {cft_strg, "root", NULL},
   {cft_strg, "ramdisk", NULL},
   {cft_flag, "read-only", NULL},
   {cft_flag, "read-write", NULL},
   {cft_strg, "append", NULL},
   {cft_strg, "initrd", NULL},

   /* Only for compatibility - unused. */
   {cft_strg, "initrd-size", NULL},
   {cft_flag, "pause-after", NULL},
   {cft_strg, "pause-message", NULL},
   {cft_strg, "init-code", NULL},
   {cft_strg, "init-message", NULL},
   {cft_end, NULL, NULL}};

CONFIG cf_image[] =
{
   {cft_strg, "image", NULL},
   {cft_strg, "label", NULL},
   {cft_strg, "alias", NULL},
   {cft_strg, "device", NULL},
   {cft_strg, "partition", NULL},
   {cft_strg, "root", NULL},
   {cft_strg, "ramdisk", NULL},
   {cft_flag, "read-only", NULL},
   {cft_flag, "read-write", NULL},
   {cft_strg, "append", NULL},
   {cft_strg, "literal", NULL},
   {cft_strg, "initrd", NULL},
   {cft_flag, "old-kernel", NULL},
   {cft_flag, "pause-after", NULL},
   {cft_strg, "pause-message", NULL},
   {cft_end, NULL, NULL}};

static char flag_set;
static char *last_token = NULL, *last_item = NULL, *last_value = NULL;
static int line_num;
static int back = 0;    /* can go back by one char */
static char *currp;
static char *endp;
static char *file_name;
static CONFIG *curr_table = cf_options;
static jmp_buf env;

static struct IMAGES {
   CONFIG table[sizeof (cf_image) / sizeof (cf_image[0])];
   struct IMAGES *next;
} *images = NULL;

void cfg_error (char *msg,...)
{
   va_list ap;

   va_start (ap, msg);
   printk ("Config file error: ");
   vprintk (msg, ap);
   va_end (ap);
   printk (" near line %d in file %s\n", line_num, file_name);
   longjmp (env, 1);
}

void cfg_warn (char *msg,...)
{
   va_list ap;

   va_start (ap, msg);
   printk ("Config file warning: ");
   vprintk (msg, ap);
   va_end (ap);
   printk (" near line %d in file %s\n", line_num, file_name);
}

static inline int getc (void)
{
   if (currp == endp) {
      return EOF;
   }

   return *currp++;
}

#define next_raw next
static int next (void)
{
   int ch;

   if (!back)
      return getc ();
   ch = back;
   back = 0;
   return ch;
}

static void again (int ch)
{
   back = ch;
}

static char *cfg_get_token (void)
{
   char buf[MAX_TOKEN + 1];
   char *here;
   int ch, escaped;

   if (last_token) {
      here = last_token;
      last_token = NULL;
      return here;
   }
   while (1) {
      while (ch = next (), ch == ' ' || ch == '\t' || ch == '\n')
         if (ch == '\n')
            line_num++;
      if (ch == EOF)
         return NULL;
      if (ch != '#')
         break;
      while (ch = next_raw (), ch != '\n')
         if (ch == EOF)
            return NULL;
      line_num++;
   }
   if (ch == '=')
      return strdup ("=");
   if (ch == '"') {
      here = buf;
      while (here - buf < MAX_TOKEN) {
         if ((ch = next ()) == EOF)
            cfg_error ("EOF in quoted string");
         if (ch == '"') {
            *here = 0;
            return strdup (buf);
         }
         if (ch == '\\') {
            ch = next ();
            switch (ch) {
            case '"':
            case '\\':
               break;
            case '\n':
               while ((ch = next ()), ch == ' ' || ch == '\t');
               if (!ch)
                  continue;
               again (ch);
               ch = ' ';
               break;
            case 'n':
               ch = '\n';
               break;
            default:
               cfg_error ("Bad use of \\ in quoted string");
            }
         } else if (ch == '\n')
            cfg_error ("newline is not allowed in quoted strings");
         *here++ = ch;
      }
      cfg_error ("Quoted string is too long");
      return 0;      /* not reached */
   }
   here = buf;
   escaped = 0;
   while (here - buf < MAX_TOKEN) {
      if (escaped) {
         if (ch == EOF)
            cfg_error ("\\ precedes EOF");
         if (ch == '\n')
            line_num++;
         else
            *here++ = ch == '\t' ? ' ' : ch;
         escaped = 0;
      } else {
         if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '#' ||
             ch == '=' || ch == EOF) {
            again (ch);
            *here = 0;
            return strdup (buf);
         }
         if (!(escaped = (ch == '\\')))
            *here++ = ch;
      }
      ch = next ();
   }
   cfg_error ("Token is too long");
   return 0;        /* not reached */
}

static void cfg_return_token(char *token)
{
   last_token = token;
}

static int cfg_next (char **item, char **value)
{
   char *this;

   if (last_item) {
      *item = last_item;
      *value = last_value;
      last_item = NULL;
      return 1;
   }
   *value = NULL;
   if (!(*item = cfg_get_token ()))
      return 0;
   if (!strcmp (*item, "="))
      cfg_error ("Syntax error");
   if (!(this = cfg_get_token ()))
      return 1;
   if (strcmp (this, "=")) {
      cfg_return_token (this);
      return 1;
   }
   if (!(*value = cfg_get_token ()))
      cfg_error ("Value expected at EOF");
   if (!strcmp (*value, "="))
      cfg_error ("Syntax error after %s", *item);
   return 1;
}

static void cfg_return (char *item, char *value)
{
   last_item = item;
   last_value = value;
}

static int cfg_set (char *item, char *value)
{
   CONFIG *walk;

   if (!strcasecmp (item, "image")) {
      struct IMAGES **p = &images;

      while (*p) {
         p = &((*p)->next);
      }

      *p = malloc (sizeof (struct IMAGES));
      (*p)->next = 0;
      bi->flags |= HAVE_IMAGES;

      curr_table = ((*p)->table);
      memcpy (curr_table, cf_image, sizeof (cf_image));
   }

   for (walk = curr_table; walk->type != cft_end; walk++) {
      if (walk->name && !strcasecmp (walk->name, item)) {
         if (value && walk->type != cft_strg)
            cfg_warn ("'%s' doesn't have a value", walk->name);
         else if (!value && walk->type == cft_strg)
            cfg_warn ("Value expected for '%s'", walk->name);
         else {
            if (walk->data)
               cfg_warn ("Duplicate entry '%s'", walk->name);
            if (walk->type == cft_flag)
               walk->data = &flag_set;
            else if (walk->type == cft_strg)
               walk->data = value;
         }

         break;
      }
   }
   if (walk->type != cft_end) {
      return 1;
   }

   cfg_return(item, value);
   return 0;
}

int cfg_parse (char *cfg_file, char *buff, int len)
{
   char *item, *value;

   file_name = cfg_file;
   currp = buff;
   endp = currp + len;

   if (setjmp (env))
      return -1;
   while (1) {
      if (!cfg_next (&item, &value))
         return 0;
      if (!cfg_set (item, value))
         return -1;
      free (item);
   }
}

static char *cfg_get_strg_i (CONFIG * table, char *item)
{
   CONFIG *walk;

   for (walk = table; walk->type != cft_end; walk++)
      if (walk->name && !strcasecmp (walk->name, item))
         return walk->data;
   return 0;
}

char *cfg_get_strg (char *image, char *item)
{
   struct IMAGES *p;
   char *label, *alias;
   char *ret;

   if (!image)
      return cfg_get_strg_i (cf_options, item);
   for (p = images; p; p = p->next) {
      label = cfg_get_strg_i (p->table, "label");
      if (!label) {
         label = cfg_get_strg_i (p->table, "image");
         alias = strrchr (label, '/');
         if (alias)
            label = alias + 1;
      }
      alias = cfg_get_strg_i (p->table, "alias");
      if (!strcmp (label, image) || (alias && !strcmp (alias, image))) {
         ret = cfg_get_strg_i (p->table, item);
         if (!ret)
            ret = cfg_get_strg_i (cf_options, item);
         return ret;
      }
   }
   return 0;
}

int cfg_get_flag (char *image, char *item)
{
   return !!cfg_get_strg (image, item);
}


void cfg_print_images(void)
{
   struct IMAGES *p;
   char *label, *alias;
   table_print_t tp;

   table_print_start(&tp, 3, 25);

   for (p = images; p; p = p->next) {
      label = cfg_get_strg_i (p->table, "label");
      if (!label) {
         label = cfg_get_strg_i(p->table, "image");
         alias = strrchr (label, '/');
         if (alias) {
            label = alias + 1;
         }
      }

      alias = cfg_get_strg_i (p->table, "alias");
      table_print(&tp, label);
      if (alias) {
         table_print(&tp, alias);
      }
   }

   table_print_end(&tp);
}

char *cfg_get_default (void)
{
   char *label;
   char *ret = cfg_get_strg_i (cf_options, "default");

   if (ret)
      return ret;
   if (!images)
      return 0;
   ret = cfg_get_strg_i (images->table, "label");
   if (!ret) {
      ret = cfg_get_strg_i (images->table, "image");
      label = strrchr (ret, '/');
      if (label)
         ret = label + 1;
   }

   return ret;
}
