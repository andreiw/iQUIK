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

#include "quik.h"
#include "prom.h"
#include "commands.h"

#define PROM_CLAIM_MAX_ADDR (0x10000000)

#define prom_getprop(node, name, buf, len)                            \
   ((int)call_prom("getprop", 4, 1, (node), (name), (buf), (len)))

ihandle prom_stdin;
ihandle prom_stdout;
phandle prom_chosen;
phandle prom_root;
phandle prom_options;
ihandle prom_mmu;
ihandle prom_memory;

/*
 * OF versions to special-case.
 */
#define OF_VER_105 "Open Firmware, 1.0.5"
#define OF_VER_201 "Open Firmware, 2.0.1"
#define OF_VER_20  "Open Firmware, 2.0"
#define OF_VER_3   "OpenFirmware 3"
#define OF_VER_NW  "iMac,1"

/*
 * Models to special-case.
 */
#define SYSTEM_3400_2400  "AAPL,3400/2400"
#define SYSTEM_WALLSTREET "AAPL,PowerBook1998"
#define SYSTEM_6400       "AAPL,e407"

/* OF 1.0.5 claim bug. */
#define PROM_CLAIM_WORK_AROUND      (1 << 1)

/* Certain bugs need us to shim away OF from the kernel. */
#define PROM_NEED_SHIM              (1 << 2)

/* OF 2.0.X on some machines - setprop doesn't do deep copy = no initrd. */
#define PROM_SHALLOW_SETPROP        (1 << 3)

/* 3400c Recent kernels hang initializing mediabay ATA. */
#define PROM_3400_HIDE_MEDIABAY_ATA (1 << 4)
#define PROM_3400_MEDIABAY          "/bandit/ohare/media-bay/ata"
#define PROM_3400_MEDIABAY_ATAPI    "/bandit/ohare/media-bay/ata/atapi-disk"
#define PROM_3400_MEDIABAY_ATA      "/bandit/ohare/media-bay/ata/ata-disk"

/* NewWorld machines need :0 after device for full disk access */
#define PROM_NW_FULL_DISK           (1 << 5)

static unsigned prom_flags = 0;
static struct prom_args prom_args;

typedef struct of_shim_state {
   /*
    * For PROM_3400_HIDE_MEDIABAY_ATA.
    */
   phandle mediabay_ata;
} of_shim_state_t;

static of_shim_state_t of_shim_state;

void (*prom_entry)(void *);


void
prom_exit()
{
   struct prom_args args;

   /*
    * TBD: Clean up opened handles.
    */

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

   if (prom_stdout == NULL) {
      return 0;
   }

   if (c == '\n')
      putchar('\r');
   return (int) call_prom("write", 3, 1, prom_stdout, &ch, 1);
}


key_t
getchar()
{
   char ch;
   int r;

   if (prom_stdin == NULL) {
      return -1;
   }

   while ((r = (int) call_prom("read", 3, 1, prom_stdin, &ch, 1)) == 0)
      ;
   return r > 0? ch: KEY_NONE;
}


int
nbgetchar()
{
   char ch;

   if (prom_stdin == NULL) {
      return -1;
   }

   return (int) call_prom("read", 3, 1, prom_stdin, &ch, 1) > 0? ch: KEY_NONE;
}


static void
prom_shim(struct prom_args *args)
{

   /*
    * Linux kernels expect setprop to be deep, so
    * the address passed can sometimes be on the stack,
    * and initrd-start is one such affected variable,
    * sadly enough.
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
           *place = (uint32_t) bi->initrd_base;
         } else if (strcmp(name, "linux,initrd-end") == 0) {
           *place = (uint32_t) bi->initrd_base +
               bi->initrd_len;
         } else {
            goto out;
         }

         *ret = 0;
         return;
      }
   }

   if (prom_flags & PROM_3400_HIDE_MEDIABAY_ATA &&
       of_shim_state.mediabay_ata != (phandle) -1) {
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

   if (bi->flags & DEBUG_BEFORE_BOOT) {
      if (strcmp(args->service, "quiesce") == 0) {
         prom_pause(NULL);
      }
   }
out:
   prom_entry(args);
}


static quik_err_t
parse_prom_flags(void)
{
   if (prom_flags & PROM_CLAIM_WORK_AROUND) {
      (void) prom_getprop(prom_chosen, "mmu", &prom_mmu, sizeof(prom_mmu));
      prom_memory = call_prom("open", 1, 1, "/memory");
      if (prom_memory == (ihandle) -1) {
         printk("couldm't open /memory for PROM_CLAIM_WORK_AROUND");
         return ERR_OF_INIT_NO_MEMORY;
      }
   }

   if (prom_flags & PROM_SHALLOW_SETPROP) {
      prom_flags |= PROM_NEED_SHIM;
   }

   if (prom_flags & PROM_3400_HIDE_MEDIABAY_ATA) {
      /*
       * Only if mediabay is empty that we hide it.
       */
      if (call_prom("finddevice", 1, 1, PROM_3400_MEDIABAY_ATAPI) == (phandle) -1 &&
          call_prom("finddevice", 1, 1, PROM_3400_MEDIABAY_ATA) == (phandle) -1) {
         of_shim_state.mediabay_ata =
            call_prom("finddevice", 1, 1, PROM_3400_MEDIABAY);
         if (of_shim_state.mediabay_ata == (phandle) -1) {
            prom_flags ^= PROM_3400_HIDE_MEDIABAY_ATA;
         } else {
            prom_flags |= PROM_NEED_SHIM;
         }
      }
   }

   if (prom_flags & PROM_NEED_SHIM) {
      bi->flags |= SHIM_OF;
   }

   return ERR_NONE;
}


quik_err_t
prom_init(void (*pp)(void *))
{
   phandle oprom;
   char ver[64];
   quik_err_t err;

   prom_entry = pp;
   bi->prom_entry  = (vaddr_t) pp;
   bi->prom_shim = (vaddr_t) prom_shim;

   prom_root = call_prom("finddevice", 1, 1, "/");
   if (prom_root == (phandle) -1) {
      return ERR_OF_INIT_NO_ROOT;
   }

   /* First get a handle for the stdout device */
   prom_chosen = call_prom("finddevice", 1, 1, "/chosen");
   if (prom_chosen == (phandle) -1) {
      return ERR_OF_INIT_NO_CHOSEN;
   }

   (void) prom_getprop(prom_chosen, "stdout", &prom_stdout, sizeof(prom_stdout));
   (void) prom_getprop(prom_chosen, "stdin", &prom_stdin, sizeof(prom_stdin));
   printk("\n");

   prom_options = call_prom("finddevice", 1, 1, "/options");
   if (prom_options == (phandle) -1) {
      return ERR_OF_INIT_NO_OPTIONS;
   }

   oprom = call_prom("finddevice", 1, 1, "/openprom");
   if (oprom == (phandle) -1) {
      return ERR_OF_INIT_NO_OPROM;
   }

   ver[0] = '\0';
   (void) prom_getprop(oprom, "model", ver, sizeof(ver));
   printk("Running on '%s'\n", ver);

   if (strcmp(ver, OF_VER_105) == 0) {
      prom_flags |= PROM_CLAIM_WORK_AROUND;
      /*
       * Not verified, but likely broken as well.
       */
      prom_flags |= PROM_SHALLOW_SETPROP;
   } else if (strcmp(ver, OF_VER_20) == 0 ||
              (strcmp(ver, OF_VER_201) == 0)) {
      /*
       * Broken for 6400, 3400c, not Wallstreet.
       * Possibly older than Wallstreet systems are affected.
       */
      prom_flags |= PROM_SHALLOW_SETPROP;
   } else if (strcmp(ver, OF_VER_3) == 0 ||
              strcmp(ver, OF_VER_NW)) {
      /*
       * All NewWorlds ar CHRP-like and need :0 to reference
       * the full device.
       */
      prom_flags |= PROM_NW_FULL_DISK;
   }

   ver[0] = '\0';
   (void) prom_getprop(prom_root, "compatible", ver, sizeof(ver));
   printk("System is '%s'\n", ver);

   if (strcmp(ver, SYSTEM_3400_2400) == 0) {
      prom_flags |= PROM_3400_HIDE_MEDIABAY_ATA;
   }

   if (strcmp(ver, SYSTEM_6400) == 0) {
      printk("PROM_NW_FULL_DISK workaround for Alchemy\n");
      prom_flags |= PROM_NW_FULL_DISK;
   }

   err = parse_prom_flags();
   if (err != ERR_NONE) {
      return err;
   }

   /* Run the preboot script if there is one. */
   if (strlen(preboot_script) != 0) {
      printk("\nPress any key in %u secs to skip preboot script ... ",
             TIMEOUT_TO_SECS(PREBOOT_TIMEOUT));
      if (wait_for_key(PREBOOT_TIMEOUT, KEY_NONE) == KEY_NONE) {
         printk("running\n");
         bi->flags |= WITH_PREBOOT;
         prom_interpret(preboot_script);
      } else {
         printk("skipped!\n");
      }
   }

   return ERR_NONE;
}


void
prom_get_chosen(char *name, char *buf, int buflen)
{
   buf[0] = 0;
   (void) prom_getprop(prom_chosen, name, buf, buflen);
}


void
prom_get_options(char *name,
                 char *buf,
                 int buflen)
{
   buf[0] = 0;
   if (prom_options != (void *) -1) {
      (void) prom_getprop(prom_options, name, buf, buflen);
   }
}


int
get_ms()
{
   return (int) call_prom("milliseconds", 0, 1);
}


void
prom_pause(char *message)
{
   if (message == NULL) {
      message = "Type go<return> to continue.\n";
   }

   printk("%s", message);
   call_prom("enter", 0, 0);
   printk("\n");
}


void
prom_interpret(char *buf)
{
  call_prom("interpret", 1, 1, buf);
}


void
set_bootargs(char *params)
{
   call_prom("setprop", 4, 1, prom_chosen, "bootargs", params,
             strlen(params) + 1);
}


void
prom_release(void *virt,
             unsigned int size)
{
   if (prom_flags & PROM_CLAIM_WORK_AROUND) {
      call_prom("call-method", 4, 0, "unmap", prom_mmu, size, virt);
      call_prom("call-method", 4, 0, "release", prom_mmu, size, virt);
      call_prom("call-method", 4, 0, "release", prom_memory, size, virt);
   }

   call_prom("release", 2, 0, virt, size);
}


void *
prom_claim(void *virt,
           unsigned int size)
{
   int ret;
   void *result;
   unsigned align = 0;

   if (prom_flags & PROM_CLAIM_WORK_AROUND) {

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
                 unsigned int size)
{
  void *found, *addr;

  for (addr = (void *) ALIGN_UP((unsigned) virt, SIZE_1M);
       addr <= (void*) PROM_CLAIM_MAX_ADDR;
       addr = (void *) ((unsigned) addr + SIZE_1M)) {
     found = prom_claim(addr, size);
     if (found != (void *)-1) {
        return found;
     }
  }

  return (void*) -1;
}


void
prom_ensure_claimed(void *virt,
                    unsigned int size)
{
   vaddr_t addr = (vaddr_t) virt;

   for (; addr < (vaddr_t) virt + size; addr += SIZE_4K) {
      prom_claim((void *) addr, SIZE_4K);
   }
}


quik_err_t
prom_open(char *device, ihandle *ih)
{
   char *d;
   ihandle c;

   d = device;
   if (prom_flags & PROM_NW_FULL_DISK) {
      d = malloc(strlen(device) + 3);
      if (d == NULL) {
         return ERR_NO_MEM;
      }

      strcpy(d, device);
      strcat(d, ":0");
   }

   c = call_prom("open", 1, 1, d);
   if (prom_flags & PROM_NW_FULL_DISK) {
      free(d);
   }

   if (c == (ihandle) 0 || c == (ihandle) -1) {
      return ERR_OF_OPEN;
   }

   *ih = c;
   return ERR_NONE;
}


static quik_err_t
cmd_prom_flags(char *args)
{
   quik_err_t err;

   if (strlen(args) == 0) {
      printk("prom_flags = 0x%x, shim = %s\n",
             prom_flags, bi->flags & SHIM_OF ? "yes" : "no");
      return ERR_NONE;
   }

   prom_flags = strtol(args, NULL, 0);

   err = parse_prom_flags();
   if (err != ERR_NONE) {
      return err;
   }

   printk("new prom_flags = 0x%x, shim = %s\n",
          prom_flags, bi->flags & SHIM_OF ? "yes" : "no");

   return ERR_NONE;
}

COMMAND(prom_flags, cmd_prom_flags, "show or set prom flags");
