/*
 * Second stage boot loader
 *
 * Copyright (C) 2013 Andrei Warkentin <andrey.warkentin@gmail.com>
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
#include <inttypes.h>
#include <string.h>
#include "elf.h"
#include "file.h"
#include "prom.h"
#include <layout.h>

#define LOW_BASE        ((void *) 0x14000)
#define LOW_END         ((void *) SECOND_BASE)
#define HIGH_BASE       ((void *) 0x800000)
#define ADDRMASK        0x0fffffff

of_shim_state_t of_shim_state;

static boot_info_t bi;

static char *pause_message = "Type go<return> to continue.\n";
static char given_bootargs[512];
static int given_bootargs_by_user = 0;

#define DEFAULT_TIMEOUT -1

void fatal(const char *msg)
{
   printk("\nFatal error: %s\n", msg);
}

void maintabfunc(boot_info_t *bi)
{
   if (bi->flags & CONFIG_VALID) {
      cfg_print_images();
      printk("boot: %s", cbuff);
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
   } else {
      *part = defpart;
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

   if (p == 0) {
      return;
   }

   while (*p == ' ') {
      ++p;
   }

   if (*p == 0) {
      *linep = NULL;
      return;
   }

   *linep = p;

   while (*p != 0 && *p != ' ') {
      ++p;
   }

   while (*p == ' ') {
      *p++ = 0;
   }

   if (*p != 0) {
      *paramsp = p;
   }
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
               char **initrd,
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

      /*
       * AndreiW:
       *
       * FIXME -
       * word_split has a screwy interface,
       * where *params could become "\0", while
       * *kname could become NULL. Ouch.
       */
      word_split(kname, params);
      if (!*kname) {
         *kname = cfg_get_default();
      }

      timeout = DEFAULT_TIMEOUT;
      if ((bi->flags & CONFIG_VALID) &&
          (q = cfg_get_strg(0, "timeout")) != 0 && *q != 0) {
         timeout = strtol(q, NULL, 0);
      }
   }

   printk("boot: ");
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
      printk("%s", *kname);
      if (*params) {
         printk(" %s", *params);
      }

      printk("\n");
   } else {
      cmdinit();
      cmdedit(maintabfunc, bi, c);
      printk("\n");
      strcpy(given_bootargs, cbuff);
      given_bootargs_by_user = 1;
      *kname = cbuff;
      word_split(kname, params);
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

      *initrd = cfg_get_strg(0, "initrd");
      p = cfg_get_strg(*kname, "image");
      if (p && *p) {
         label = *kname;
         *kname = p;

         p = cfg_get_strg(label, "device");
         if (p) {
            defdevice = p;
         }

         p = cfg_get_strg(label, "partition");
         if (p) {
            n = strtol(p, &endp, 10);
            if (endp != p && *endp == 0) {
               defpart = n;
            }
         }

         p = cfg_get_strg(label, "initrd");
         if (p) {
            *initrd = p;
         }

         if (cfg_get_strg(label, "old-kernel")) {
            bi->flags |= BOOT_PRE_2_4;
         } else {
            bi->flags &= ~BOOT_PRE_2_4;
         }

         *params = make_params(bi, label, *params);
      }
   }

   if (!strcmp(*kname, "!debug")) {
      bi->flags |= DEBUG_BEFORE_BOOT;
      *kname = NULL;
      return 0;
   } else if (!strcmp(*kname, "!shim")) {
      bi->flags |= SHIM_OF;
      printk("WILL SHIM\n");
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

   if (!*kname) {
      printk(
         "Enter the kernel image name as [device:][partno]/path, where partno is a\n"
         "number from 0 to 16.  Instead of /path you can type [mm-nn] to specify a\n"
         "range of disk blocks (512B)\n");
   }

   return 0;
}

/*
 * Print the specified message file.
 */
static void
print_message_file(boot_info_t *bi, char *p)
{
   char *q, *endp;
   int len = 0;
   char *message;
   int n, defpart = bi->config_part;
   char *device, *kname;
   int part;

   q = cfg_get_strg(0, "partition");
   if (q) {
      n = strtol(q, &endp, 10);
      if (endp != q && *endp == 0) {
         defpart = n;
      }
   }

   parse_name(p, defpart, &device, &part, &kname);
   if (kname) {
      if (!device) {
         device = cfg_get_strg(0, "device");
      }

      message = LOW_BASE;
      if (load_file(device, part, kname, LOW_BASE, LOW_END, &len, 1, 0)) {
         message[len] = 0;
         printk("\n%s", message);
      }
   }
}

int get_bootargs(boot_info_t *bi)
{
   prom_get_chosen("bootargs", bi->of_bootargs, sizeof(bi->of_bootargs));
   printk("Passed arguments: '%s'\n", bi->of_bootargs);
   bi->bootargs = bi->of_bootargs;
   return 0;
}

/* Here we are launched */
int main(void *prom_entry, struct first_info *fip, unsigned long id)
{
   unsigned off;
   int i, len;
   fs_len_t image_len, initrd_len;
   char *kname, *initrd, *params, *device;
   unsigned part;
   int fileok = 0;
   Elf32_Ehdr *e;
   Elf32_Phdr *p;
   char *load_buf;
   char *load_buf_end;
   unsigned load_loc, entry, start, initrd_base;
   extern char __bss_start, _end;
   quik_err_t err;

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

   prom_init(prom_entry, &bi);
   printk("iQUIK OldWorld Bootloader\n");
   printk("Copyright (C) 2013 Andrei Warkentin <andrey.warkentin@gmail.com>\n");
   if (bi.flags & SHIM_OF) {
      printk("This firmware requires a shim to work around bugs.\n");
   }

   get_bootargs(&bi);

   diskinit(&bi);
   if (!bi.bootdevice ||
       bi.config_part == 0) {
      printk("Skipping loading configuration file due to missing or invalid configuration.\n");
   } else {
      int len;

      printk("Configuration file @ %s:%d%s\n", bi.bootdevice,
             bi.config_part,
             bi.config_file);

      fileok = load_file(bi.bootdevice,
                         bi.config_part,
                         bi.config_file,
                         LOW_BASE,
                         LOW_END,
                         &len, 1, NULL);
      if (!fileok || (unsigned) len >= 65535) {
         printk("\nCouldn't load '%s'.\n", bi.config_file);
      } else {
         char *p;
         if (cfg_parse(bi.config_file, LOW_BASE, len) < 0) {
            printk ("Syntax error or read error in %s.\n", bi.config_file);
         }

         bi.flags |= CONFIG_VALID;
         p = cfg_get_strg(0, "init-code");
         if (p) {
            call_prom("interpret", 1, 1, p);
         }

         p = cfg_get_strg(0, "init-message");
         if (p) {
            printk("%s\n", p);
         }

         p = cfg_get_strg(0, "message");
         if (p) {
            print_message_file(&bi, p);
         }
      }
   }

   for (;;) {
      quik_err_t err;

      get_params(&bi, &device, &part,
                 &kname, &initrd, &params);
      if (!kname) {
         continue;
      }

      printk("Loading %s\n", kname);
      err = length_file(device, part, kname, &image_len);
      if (err != ERR_NONE) {
         printk("Error fetching size for '%s': %r\n", kname, err);
         continue;
      }

      if ((image_len > (LOW_END - LOW_BASE)) &&
          (bi.flags & BOOT_PRE_2_4)) {
         printk("Kernel '%s' too large to be loaded low and too old to be loaded high\n",
                kname);
         continue;
      }

      if (bi.flags & BOOT_PRE_2_4) {
         load_buf = LOW_BASE;
         load_buf_end = LOW_END;
      } else {
         load_buf = prom_claim_chunk(HIGH_BASE, image_len, 0);
         if (load_buf == (char *) -1) {
            printk("Couldn't allocate memory to load kernel.\n");
            continue;
         }
         printk("kernel is at %p\n", load_buf);

         load_buf_end = load_buf + image_len;
      }

      fileok = load_file(device, part, kname,
                         load_buf, load_buf_end,
                         &image_len, 1, NULL);

      if (!fileok) {
         printk("\nImage '%s' not found.\n", kname);
         continue;
      }


      /* By this point the first sector is loaded (and the rest of */
      /* the kernel) so we check if it is an executable elf binary. */

      e = (Elf32_Ehdr *) load_buf;
      if (!(e->e_ident[EI_MAG0] == ELFMAG0 &&
            e->e_ident[EI_MAG1] == ELFMAG1 &&
            e->e_ident[EI_MAG2] == ELFMAG2 &&
            e->e_ident[EI_MAG3] == ELFMAG3)) {
         printk("\n%s: unknown image format\n", kname);
         continue;
      }

      if (e->e_ident[EI_CLASS] != ELFCLASS32
          || e->e_ident[EI_DATA] != ELFDATA2MSB) {
         printk("Image is not a 32bit MSB ELF image\n");
         continue;
      }

      len = 0;
      p = (Elf32_Phdr *) (load_buf + e->e_phoff);
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
         printk("Cannot find a loadable segment in ELF image\n");
         continue;
      }

      entry = e->e_entry & ADDRMASK;
      if (len + off > image_len) {
         len = image_len - off;
      }

      if (!initrd) {
         initrd_base = 0;
         initrd_len = 0;
         break;
      }

      printk("Loading %s\n", initrd);

      err = length_file(device, part, initrd, &initrd_len);
      if (err != ERR_NONE) {
         printk("Error fetching size for '%s': %r\n", initrd, err);
         continue;
      }

      if ((void *) load_buf >= HIGH_BASE) {
         initrd_base = (unsigned) load_buf_end;
      } else {
         initrd_base = (unsigned) HIGH_BASE;
      }

      initrd_base = (unsigned) prom_claim_chunk((void *) initrd_base, initrd_len, 0);
      if (initrd_base == (unsigned) -1) {
         printk("Claim failed\n");
         continue;
      }

      fileok = load_file(device, part, initrd,
                         initrd_base,
                         initrd_base + initrd_len,
                         &initrd_len, 1, NULL);
      if (!fileok) {
         printk("Initrd '%s' not found.\n", initrd);
         continue;
      }

      break;
   }

   if (bi.flags & BOOT_PRE_2_4) {

      /*
       * 2.2 kernels have to execute at PA = 0x0 on the PowerMac.
       *
       * After this memmove, *p and *e may have been overwritten.
       */
      memmove((void *)load_loc, load_buf + off, len);
   } else {

      /*
       * Newer kernels can relocate themselves just fine, so we
       * always load them high - they'll probably not fit in the low region
       * anyway.
       */
     load_loc = (unsigned) load_buf + off;
   }

   flush_cache(load_loc, len);

   close();

   if (bi.flags & SHIM_OF) {
      of_shim_state.initrd_base = initrd_base;
      of_shim_state.initrd_len = initrd_len;
   }

   if (bi.flags & DEBUG_BEFORE_BOOT) {
      if (bi.flags & BOOT_PRE_2_4) {
         printk("Booting pre-2.4 kernel\n");
      }

      printk("Kernel: 0x%x @ 0x%x\n", image_len, load_loc);
      printk("Initrd: 0x%x @ 0x%x\n", initrd_len, initrd_base);
      printk("Kernel parameters: %s\n", params);
      printk(pause_message);
      prom_pause();
      printk("\n");
   } else if (bi.flags & PAUSE_BEFORE_BOOT) {
      printk("%s", bi.pause_message);
      prom_pause();
      printk("\n");
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

   printk("Starting at %x\n", start);
   set_bootargs(params);
   (* (void (*)()) start)(initrd_base, initrd_len,
                          (bi.flags & SHIM_OF) ?
                          prom_shim :
                          prom_entry,
                          0, 0);

   prom_exit();
}
