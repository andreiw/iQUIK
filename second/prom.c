/*
 * Procedures for interfacing to the Open Firmware PROM on
 * Power Macintosh computers.
 *
 * Paul Mackerras October 1996.
 * Copyright (C) 1996 Paul Mackerras.
 */

#include <stdarg.h>
#include "prom.h"

#define getpromprop(node, name, buf, len)                            \
   ((int)call_prom("getprop", 4, 1, (node), (name), (buf), (len)))

ihandle prom_stdin;
ihandle prom_stdout;
ihandle prom_chosen;
ihandle prom_options;

struct prom_args {
   char *service;
   int nargs;
   int nret;
   void *args[10];
} prom_args;

void (*prom_entry)(void *);

void
prom_exit()
{
   struct prom_args args;

   args.service = "exit";
   args.nargs = 0;
   args.nret = 0;
   prom_entry(&args);
   for (;;)         /* should never get here */
      ;
}

void *
call_prom(char *service, int nargs, int nret, ...)
{
   va_list list;
   int i;

   prom_args.service = service;
   prom_args.nargs = nargs;
   prom_args.nret = nret;
   va_start(list, nret);
   for (i = 0; i < nargs; ++i)
      prom_args.args[i] = va_arg(list, void *);
   va_end(list);
   for (i = 0; i < nret; ++i)
      prom_args.args[i + nargs] = 0;
   prom_entry(&prom_args);
   return prom_args.args[nargs];
}

void
prom_print(char *msg)
{
   char *p, *q;
   char *crlf = "\r\n";

   for (p = msg; *p != 0; p = q) {
      for (q = p; *q != 0 && *q != '\n'; ++q)
         ;
      if (q > p)
         call_prom("write", 3, 1, prom_stdout, p, q - p);
      if (*q != 0) {
         ++q;
         call_prom("write", 3, 1, prom_stdout, crlf, 2);
      }
   }
}

int
putchar(int c)
{
   char ch = c;

   if (c == '\n')
      putchar('\r');
   return (int) call_prom("write", 3, 1, prom_stdout, &ch, 1);
}

int
getchar()
{
   char ch;
   int r;

   while ((r = (int) call_prom("read", 3, 1, prom_stdin, &ch, 1)) == 0)
      ;
   return r > 0? ch: -1;
}

int
nbgetchar()
{
   char ch;

   return (int) call_prom("read", 3, 1, prom_stdin, &ch, 1) > 0? ch: -1;
}

void
prom_init(void (*pp)(void *))
{
   prom_entry = pp;
   /* First get a handle for the stdout device */
   prom_chosen = call_prom("finddevice", 1, 1, "/chosen");
   if (prom_chosen == (void *)-1)
      prom_exit();
   getpromprop(prom_chosen, "stdout", &prom_stdout, sizeof(prom_stdout));
   getpromprop(prom_chosen, "stdin", &prom_stdin, sizeof(prom_stdin));
   prom_options = call_prom("finddevice", 1, 1, "/options");
}

void
prom_get_chosen(char *name, char *buf, int buflen)
{
   buf[0] = 0;
   getpromprop(prom_chosen, name, buf, buflen);
}

void
prom_get_options(char *name, char *buf, int buflen)
{
   buf[0] = 0;
   if (prom_options != (void *) -1)
      getpromprop(prom_options, name, buf, buflen);
}

int
get_ms()
{
   return (int) call_prom("milliseconds", 0, 1);
}

void
prom_pause()
{
   call_prom("enter", 0, 0);
}

void
set_bootargs(char *params)
{
   call_prom("setprop", 4, 1, prom_chosen, "bootargs", params,
             strlen(params) + 1);
}
