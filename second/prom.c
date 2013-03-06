/*
 * Procedures for interfacing to the Open Firmware PROM on
 * Power Macintosh computers.
 *
 * Copyright (C) Andrei Warkentin <andrey.warkentin>
 *
 * Portions from yaboot, so:

 * Copyright (C) 2001, 2002 Ethan Benson
 * Copyright (C) 1999 Benjamin Herrenschmidt
 * Copyright (C) 1999 Marius Vollmer
 * Copyright (C) 1996 Paul Mackerras.
 *
 * Portion from Linux kernel prom_init.c, so:
 */

#include <stdarg.h>
#include "prom.h"

#define PROM_CLAIM_MAX_ADDR (0x10000000)

#define getpromprop(node, name, buf, len)                            \
   ((int)call_prom("getprop", 4, 1, (node), (name), (buf), (len)))

ihandle prom_stdin;
ihandle prom_stdout;
ihandle prom_chosen;
ihandle prom_aliases;
ihandle prom_options;
ihandle prom_mmu;
ihandle prom_memory;

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

   prom_memory = call_prom("open", 1, 1, "/memory");
   getpromprop(prom_chosen, "mmu", &prom_mmu, sizeof(prom_mmu));
   prom_aliases = call_prom("finddevice", 1, 1, "/aliases");
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
prom_get_alias(char *name, char *buf, int buflen)
{
   buf[0] = 0;
   if (prom_aliases != (void *) -1) {
      return getpromprop(prom_aliases, name, buf, buflen);
   }

   return 0;
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

void *
prom_claim(void *virt, unsigned int size, unsigned int align)
{
   int ret;
   void *result;

   /*
    * Old OF requires we claim physical and virtual separately
    * and then map explicitly (assuming virtual mode)
    */

   ret = (int) call_prom("call-method", 5, 2, &result,
                         "claim", prom_memory,
                         align, size, virt);
   if (ret != 0 || result == (void *) -1) {
      return (void *) -1;
   }

   ret = (int) call_prom("call-method", 5, 2, &result,
                         "claim", prom_mmu,
                         align, size, virt);
   if (ret != 0) {
      call_prom("call-method", 4, 1, "release",
                prom_memory, size, virt);
      return (void *) -1;
   }

   /* the 0x12 is M (coherence) + PP == read/write */
   call_prom("call-method", 6, 1,
             "map", prom_mmu, 0x12, size, virt, virt);

   return virt;

   /* return call_prom("claim", 3, 1, virt, size, align); */
}

/*
 * if address given is claimed look for other addresses to get the needed
 * space before giving up.
 */
void *
prom_claim_chunk(void *virt,
                 unsigned int size,
                 unsigned int align)
{
  void *found, *addr;

  for(addr = virt;
      addr <= (void*) PROM_CLAIM_MAX_ADDR;
      addr += (0x100000/sizeof(addr))) {

     found = prom_claim(addr, size, 0);
     if (found != (void *)-1) {
        printk("claimed %u at 0x%x (expected 0x%x)\n",size, (int) found, (int) virt);
        return found;
     }
  }

  return (void*) -1;
}
