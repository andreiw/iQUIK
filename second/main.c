/*
 * Second stage boot loader
 *
 * Copyright (C) 1996 Paul Mackerras.
 *
 * Because this program is derived from the corresponding file in the
 * silo-0.64 distribution, it is also
 *
 * Copyright (C) 1996 Pete A. Zaitcev
 * 1996 Maurizio Plaza
 * 1996 David S. Miller
 * 1996 Miguel de Icaza
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
#include <string.h>
#define __KERNEL__
#include <linux/elf.h>
#include <layout.h>

#define TMP_BUF         ((unsigned char *) 0x14000)
#define TMP_END         ((unsigned char *) SECOND_BASE)
#define ADDRMASK        0x0fffffff

static boot_info_t bi;

static char *pause_message = "Type go<return> to continue.\n";
static char given_bootargs[512];
static int given_bootargs_by_user = 0;

#define DEFAULT_TIMEOUT         -1

void fatal(const char *msg)
{
   printf("\nFatal error: %s\n", msg);
}

void maintabfunc(boot_info_t *bi)
{
   if (bi->flags & CONFIG_VALID) {
      cfg_print_images();
      printf("boot: %s", cbuff);
   }
}

void parse_name(char *imagename,
                int defpart,
                char **device,
                unsigned *part,
                char **kname)
{
   int n;
   char *endp;

   /*
    * Assume partition 2 if no other has been explicitly set
    * in the config file -- Cort
    */
   if (defpart == -1) {
      defpart = 2;
      printf("no parition explicitely set - assuming 2\n");
   }

   *kname = strchr(imagename, ':');
   if (!*kname) {
      *kname = imagename;
      *device = 0;
   } else {
      **kname = 0;
      (*kname)++;
      *device = imagename;
   }

   n = strtol(*kname, &endp, 0);
   if (endp != *kname) {
      *part = n;
      *kname = endp;
   } else if (defpart != -1) {
      *part = defpart;
   } else {
      *part = 0;
   }

   /* Range */
   if (**kname == '[') {
      return;
   }

   /* Path */
   if (**kname != '/') {
      *kname = 0;
   }
}

void
word_split(char **linep, char **paramsp)
{
   char *p;

   *paramsp = "\0";
   p = *linep;
   if (p == 0)
      return;
   while (*p == ' ')
      ++p;
   if (*p == 0) {
      *linep = 0;
      return;
   }
   *linep = p;
   while (*p != 0 && *p != ' ')
      ++p;
   while (*p == ' ')
      *p++ = 0;
   if (*p != 0)
      *paramsp = p;
}

char *
make_params(boot_info_t *bi,
            char *label,
            char *params)
{
   char *p, *q;
   static char buffer[2048];

   q = buffer;
   *q = 0;

   p = cfg_get_strg(label, "literal");
   if (p) {
      strcpy(q, p);
      q = strchr(q, 0);
      if (*params) {
         if (*p)
            *q++ = ' ';
         strcpy(q, params);
      }
      return buffer;
   }

   p = cfg_get_strg(label, "root");
   if (p) {
      strcpy (q, "root=");
      strcpy (q + 5, p);
      q = strchr (q, 0);
      *q++ = ' ';
   }
   if (cfg_get_flag(label, "read-only")) {
      strcpy (q, "ro ");
      q += 3;
   }
   if (cfg_get_flag(label, "read-write")) {
      strcpy (q, "rw ");
      q += 3;
   }
   p = cfg_get_strg(label, "ramdisk");
   if (p) {
      strcpy (q, "ramdisk=");
      strcpy (q + 8, p);
      q = strchr (q, 0);
      *q++ = ' ';
   }
   p = cfg_get_strg (label, "append");
   if (p) {
      strcpy (q, p);
      q = strchr (q, 0);
      *q++ = ' ';
   }
   *q = 0;

   if (cfg_get_flag (label, "pause-after")) {
      bi->flags |= PAUSE_BEFORE_BOOT;
   }

   p = cfg_get_strg(label, "pause-message");
   if (p) {
      bi->pause_message = p;
   }

   if (*params) {
      strcpy(q, params);
   }

   return buffer;
}

int get_params(boot_info_t *bi,
               char **device,
               unsigned *part,
               char **kname,
               char **params)
{
   char *defdevice = 0;
   char *p, *q, *endp;
   int c, n;
   char *label;
   int timeout = -1;
   int beg = 0, end;
   unsigned defpart = bi->config_part;

   if ((bi->flags & TRIED_AUTO) == 0) {
      bi->flags ^= TRIED_AUTO;
      *params = bi->bootargs;
      *kname = *params;
      *device = bi->bootdevice;
      word_split(kname, params);

      timeout = DEFAULT_TIMEOUT;
      if ((bi->flags & CONFIG_VALID) &&
          (q = cfg_get_strg(0, "timeout")) != 0 && *q != 0) {
         timeout = strtol(q, NULL, 0);
      }
   }

   printf("boot: ");
   c = -1;
   if (timeout != -1) {
      beg = get_ms();
      if (timeout > 0) {
         end = beg + 100 * timeout;
         do {
            c = nbgetchar();
         } while (c == -1 && get_ms() <= end);
      }
      if (c == -1) {
         c = '\n';
      }
   }

   if (c == '\n') {
      printf("%s", *kname);
      if (*params) {
         printf(" %s", *params);
      }

      printf("\n");
   } else {
      cmdinit();
      cmdedit(maintabfunc, bi, c);
      printf("\n");
      strcpy(given_bootargs, cbuff);
      given_bootargs_by_user = 1;
      *kname = cbuff;
      word_split(&*kname, &*params);
   }

   label = 0;
   defdevice = *device;

   if (bi->flags & CONFIG_VALID) {
      if(cfg_get_strg(0, "device") != NULL) {
         defdevice = cfg_get_strg(0, "device");
      }

      p = cfg_get_strg(0, "partition");
      if (p) {
         n = strtol(p, &endp, 10);
         if (endp != p && *endp == 0)
            defpart = n;
      }

      p = cfg_get_strg(0, "pause-message");
      if (p) {
         bi->pause_message = p;
      }

      p = cfg_get_strg(*kname, "image");
      if (p && *p) {
         label = *kname;
         *kname = p;

         if (cfg_get_strg(label, "device") != NULL) {
            defdevice = cfg_get_strg(label, "device");
         }

         p = cfg_get_strg(label, "partition");
         if (p) {
            n = strtol(p, &endp, 10);
            if (endp != p && *endp == 0) {
               defpart = n;
            }
         }

         *params = make_params(bi, label, *params);
      }
   }

   if (!strcmp(*kname, "!debug")) {
      bi->flags |= DEBUG_BEFORE_BOOT;
      *kname = NULL;
      return 0;
   } else if (!strcmp(*kname, "!halt")) {
      prom_pause();
      *kname = NULL;
      return 0;
   } else if (*kname[0] == '$') {

      /* forth command string */
      call_prom("interpret", 1, 1, *kname + 1);
      *kname = NULL;
      return 0;
   }

   parse_name(*kname, defpart, device, part, kname);
   if (!*device) {
      *device = defdevice;
   }

   if (!*kname)
      printf(
         "Enter the kernel image name as [device:][partno]/path, where partno is a\n"
         "number from 0 to 16.  Instead of /path you can type [mm-nn] to specify a\n"
         "range of disk blocks (512B)\n");

   return 0;
}

/*
 * Print the specified message file.
 */
static void
print_message_file(char *p)
{
   char *q, *endp;
   int len = 0;
   int n, defpart = -1;
   char *device, *kname;
   int part;

   q = cfg_get_strg(0, "partition");
   if (q) {
      n = strtol(q, &endp, 10);
      if (endp != q && *endp == 0)
         defpart = n;
   }
   parse_name(p, defpart, &device, &part, &kname);
   if (kname) {
      if (!device)
         device = cfg_get_strg(0, "device");
      if (load_file(device, part, kname, TMP_BUF, TMP_END, &len, 1, 0)) {
         TMP_BUF[len] = 0;
         printf("\n%s", (char *)TMP_BUF);
      }
   }
}

int get_bootargs(boot_info_t *bi)
{
   prom_get_chosen("bootargs", bi->of_bootargs, sizeof(bi->of_bootargs));
   printf("Passed arguments: '%s'\n", bi->of_bootargs);
   bi->bootargs = bi->of_bootargs;
   return 0;
}

/* Here we are launched */
int main(void *prom_entry, struct first_info *fip, unsigned long id)
{
   unsigned off;
   int i, len, image_len;
   char *kname, *params, *device;
   unsigned part;
   int isfile, fileok = 0;
   unsigned int ret_offset;
   Elf32_Ehdr *e;
   Elf32_Phdr *p;
   unsigned load_loc, entry, start;
   extern char __bss_start, _end;

   /* Always first. */
   memset(&__bss_start, 0, &_end - &__bss_start);

   /*
    * If we're not being called by the first stage bootloader,
    * then we must have booted from OpenFirmware directly, via
    * bootsector 0.
    */
   if (id != 0xdeadbeef) {
      bi.flags |= BOOT_FROM_SECTOR_ZERO;

      /* OF passes prom_entry in r5 */
      prom_entry = (void *) id;
   } else {
      bi.config_file = fip->conf_file;
      bi.config_part = fip->conf_part;
   }

   prom_init(prom_entry);
   printf("\niQUIK OldWorld Bootloader\nCopyright (C) 2013 Andrei Warkentin\n");
   get_bootargs(&bi);

   /*
    * AndreiW: FIXME, let the user enter a boot device path.
    */
   if (diskinit(&bi) == -1) {
      printf("Could not determine the boot device\n");
      prom_exit();
   }

   printf("Configuration file @ %s:%d%s\n", bi.bootdevice,
          bi.config_part,
          bi.config_file);

   if (bi.config_file &&
       bi.config_part >= 0) {
      int len;

      fileok = load_file(bi.bootdevice,
                         bi.config_part,
                         bi.config_file,
                         TMP_BUF,
                         TMP_END,
                         &len, 1, 0);
      if (!fileok || (unsigned) len >= 65535) {
         printf("\nCouldn't load '%s'.\n", bi.config_file);
      } else {
         char *p;
         if (cfg_parse(bi.config_file, TMP_BUF, len) < 0) {
            printf ("Syntax error or read error in %s.\n", bi.config_file);
         }

         bi.flags |= CONFIG_VALID;
         p = cfg_get_strg(0, "init-code");
         if (p) {
            call_prom("interpret", 1, 1, p);
         }

         p = cfg_get_strg(0, "init-message");
         if (p) {
            printf("%s\n", p);
         }

         p = cfg_get_strg(0, "message");
         if (p) {
            print_message_file(p);
         }
      }
   } else {
      printf ("\n");
   }

   for (;;) {
      get_params(&bi, &device, &part, &kname, &params);
      if (!kname) {
         continue;
      }

      fileok = load_file(device, part, kname,
                         TMP_BUF, TMP_END, &image_len, 1, 0);

      if (!fileok) {
         printf ("\nImage '%s' not found.\n", kname);
         continue;
      }

      if (image_len > TMP_END - TMP_BUF) {
         printf("\nImage is too large (%u > %u)\n", image_len,
                TMP_END - TMP_BUF);
         continue;
      }

      /* By this point the first sector is loaded (and the rest of */
      /* the kernel) so we check if it is an executable elf binary. */

      e = (Elf32_Ehdr *) TMP_BUF;
      if (!(e->e_ident[EI_MAG0] == ELFMAG0 &&
            e->e_ident[EI_MAG1] == ELFMAG1 &&
            e->e_ident[EI_MAG2] == ELFMAG2 &&
            e->e_ident[EI_MAG3] == ELFMAG3)) {
         printf ("\n%s: unknown image format\n", kname);
         continue;
      }

      if (e->e_ident[EI_CLASS] != ELFCLASS32
          || e->e_ident[EI_DATA] != ELFDATA2MSB) {
         printf("Image is not a 32bit MSB ELF image\n");
         continue;
      }

      len = 0;
      p = (Elf32_Phdr *) (TMP_BUF + e->e_phoff);
      for (i = 0; i < e->e_phnum; ++i, ++p) {
         if (p->p_type != PT_LOAD || p->p_offset == 0)
            continue;
         if (len == 0) {
            off = p->p_offset;
            len = p->p_filesz;
            load_loc = p->p_vaddr & ADDRMASK;
         } else
            len = p->p_offset + p->p_filesz - off;
      }

      if (len == 0) {
         printf("Cannot find a loadable segment in ELF image\n");
         continue;
      }

      entry = e->e_entry & ADDRMASK;
      if (len + off > image_len) {
         len = image_len - off;
      }

      break;
   }

   /* After this memmove, *p and *e may have been overwritten. */
   memmove((void *)load_loc, TMP_BUF + off, len);
   flush_cache(load_loc, len);

   close();
   if (bi.flags & DEBUG_BEFORE_BOOT) {
      printf("Load location: 0x%x\n", load_loc);
      printf("Kernel parameters: %s\n", params);
      printf(pause_message);
      prom_pause();
      printf("\n");
   } else if (bi.flags & PAUSE_BEFORE_BOOT) {
      printf("%s", bi.pause_message);
      prom_pause();
      printf("\n");
   }

   /*
    * For the sake of the Open Firmware XCOFF loader, the entry
    * point may actually be a procedure descriptor.
    */
   start = *(unsigned *) entry;

   /* new boot strategy - see head.S in the kernel for more info -- Cort */
   if (start == 0x60000000) {
      /* nop */

      start = load_loc;
   } else {
      /* not the new boot strategy, use old logic -- Cort */

      if (start < load_loc || start >= load_loc + len
          || ((unsigned *)entry)[2] != 0) {

         /* doesn't look like a procedure descriptor */
         start += entry;
      }
   }
   printf("Starting at %x\n", start);

   (* (void (*)()) start)(params, 0, prom_entry, 0, 0);
   prom_exit();
}
