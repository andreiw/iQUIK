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
 * Copyright (C) 1996-2005 Paul Mackerras.
 */

#include <stdarg.h>
#include "quik.h"
#include "prom.h"

#define PROM_CLAIM_MAX_ADDR (0x10000000)

#define prom_getprop(node, name, buf, len)                            \
   ((int)call_prom("getprop", 4, 1, (node), (name), (buf), (len)))

ihandle prom_stdin;
ihandle prom_stdout;
ihandle prom_chosen;
ihandle prom_aliases;
ihandle prom_options;
ihandle prom_mmu;
ihandle prom_memory;

/* OF 1.0.5 claim bug. */
#define PROM_CLAIM_WORK_AROUND    (1 << 1)

/* Certain bugs need us to shim away OF from the kernel. */
#define PROM_NEED_SHIM            (1 << 2)

/* OF 2.0.1 setprop doesn't do deep copy = no initrd. */
#define PROM_SHALLOW_SETPROP      (1 << 3)

/* 3400c or OF 2.0.1? Recent kernels hang initializing mediabay ATA. */
#define PROM_HIDE_MEDIABAY_ATA    (1 << 4)
static unsigned prom_flags = 0;

static struct prom_args prom_args;
of_shim_state_t of_shim_state;

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


static void
prom_shim(struct prom_args *args)
{

   /*
    * Linux kernels expect setprop to be deep, so
    * the address passed can sometimes be on the stack,
    * and initrd-start is one such affected variable,
    * sadly enough.
    *
    * This should be fixed in the kernel, but before it is,
    * older kernels still need to be able to boot.
    */
   if (prom_flags & PROM_SHALLOW_SETPROP) {
      if (strcmp(args->service, "getprop") == 0) {
         ihandle ih = (ihandle) args->args[0];
         char *name = (char *) args->args[1];
         unsigned *place = (unsigned *) args->args[2];
         unsigned *ret = (unsigned *) args->args[4];

         if (ih != prom_chosen) {
            goto out;
         }

         if (strcmp(name, "linux,initrd-start") == 0) {
           *place = (uint32_t) of_shim_state.initrd_base;
         } else if (strcmp(name, "linux,initrd-end") == 0) {
           *place = (uint32_t) of_shim_state.initrd_base +
               of_shim_state.initrd_len;
         } else {
            goto out;
         }

         *ret = 0;
         return;
      }
   }

   if (prom_flags & PROM_HIDE_MEDIABAY_ATA) {
      if (strcmp(args->service, "child") == 0 ||
          strcmp(args->service, "peer") == 0 ||
          strcmp(args->service, "parent") == 0) {

         prom_entry(args);
         if (args->args[args->nargs] ==
             of_shim_state.mediabay_ata) {
            args->args[args->nargs] = 0;
         }

         return;
      }
   }

#if 0
   if (strcmp(args->service, "quiesce") == 0) {
      printk("'go' to continue kernel boot\n");
      prom_pause();
   }
#endif

out:
   prom_entry(args);
}


void
prom_init(void (*pp)(void *),
          boot_info_t *bi)
{
   ihandle oprom;
   char ver[64];

   prom_entry = pp;
   bi->prom_entry  = (vaddr_t) pp;
   bi->prom_shim = (vaddr_t) prom_shim;

   /* First get a handle for the stdout device */
   prom_chosen = call_prom("finddevice", 1, 1, "/chosen");
   if (prom_chosen == (void *) -1) {
      prom_exit();
   }

   prom_memory = call_prom("open", 1, 1, "/memory");
   prom_getprop(prom_chosen, "mmu", &prom_mmu, sizeof(prom_mmu));
   prom_aliases = call_prom("finddevice", 1, 1, "/aliases");
   prom_getprop(prom_chosen, "stdout", &prom_stdout, sizeof(prom_stdout));
   prom_getprop(prom_chosen, "stdin", &prom_stdin, sizeof(prom_stdin));
   prom_options = call_prom("finddevice", 1, 1, "/options");
   printk("\n");

   ver[0] = '\0';
   oprom = call_prom("finddevice", 1, 1, "/openprom");
   prom_getprop(oprom, "model", ver, sizeof(ver));
   if (strcmp(ver, "Open Firmware, 1.0.5") == 0) {
      prom_flags |= PROM_CLAIM_WORK_AROUND;
   } else if (strcmp(ver, "Open Firmware, 2.0.1") == 0) {
      prom_flags |= PROM_NEED_SHIM;
      prom_flags |= PROM_SHALLOW_SETPROP;
      prom_flags |= PROM_HIDE_MEDIABAY_ATA;
      of_shim_state.mediabay_ata =
         call_prom("finddevice", 1, 1, "/bandit/ohare/media-bay/ata");
   }

   if (prom_flags & PROM_NEED_SHIM) {
      bi->flags |= SHIM_OF;
   }

   prom_get_chosen("bootargs", bi->of_bootargs, sizeof(bi->of_bootargs));
   printk("Passed arguments: '%s'\n", bi->of_bootargs);
   bi->bootargs = bi->of_bootargs;
}


void
prom_get_chosen(char *name, char *buf, int buflen)
{
   buf[0] = 0;
   prom_getprop(prom_chosen, name, buf, buflen);
}


void
prom_get_options(char *name,
                 char *buf,
                 int buflen)
{
   buf[0] = 0;
   if (prom_options != (void *) -1)
      prom_getprop(prom_options, name, buf, buflen);
}


int
prom_get_alias(char *name,
               char *buf,
               int buflen)
{
   buf[0] = 0;
   if (prom_aliases != (void *) -1) {
      return prom_getprop(prom_aliases, name, buf, buflen);
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
prom_claim(void *virt,
           unsigned int size,
           unsigned int align)
{
   int ret;
   void *result;

   if ((prom_flags & PROM_CLAIM_WORK_AROUND) &&
       align == 0) {

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
   }

   return call_prom("claim", 3, 1, virt, size, align);
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

  for (addr = (void *) ALIGN_UP((unsigned) virt, SIZE_1M);
       addr <= (void*) PROM_CLAIM_MAX_ADDR;
       addr = (void *) ((unsigned) addr + SIZE_1M)) {
     found = prom_claim(addr, size, 0);
     if (found != (void *)-1) {
        return found;
     }

     printk("\ncouldn't claim at %p\n", addr);
  }

  return (void*) -1;
}
