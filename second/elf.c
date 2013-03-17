/*
 * ELF manipulation.
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

#include "quik.h"
#include "elf.h"

#define ADDRMASK 0x0fffffff

/*
 * Given a buffer, parse out:
 * - The actual linked address, or the address that we think is one.
 *   (actual ELF one for old kernels, and code start for new kernels
 *    since they can run anywhere).
 * - Offset within load_buf where code starts.
 * - Size of this code.
 * - Entry point.
 */
quik_err_t elf_parse(void *load_buf,
                     length_t load_buf_len,
                     load_state_t *image)
{
   unsigned i;
   Elf32_Ehdr *e;
   Elf32_Phdr *p;
   length_t len;
   offset_t off;
   vaddr_t entry;
   vaddr_t linked_base;

   memset(image, 0, sizeof(*image));

   e = (Elf32_Ehdr *) load_buf;
   if (!(e->e_ident[EI_MAG0] == ELFMAG0 &&
         e->e_ident[EI_MAG1] == ELFMAG1 &&
         e->e_ident[EI_MAG2] == ELFMAG2 &&
         e->e_ident[EI_MAG3] == ELFMAG3)) {
      return ERR_ELF_NOT;
   }

   if (e->e_ident[EI_CLASS] != ELFCLASS32
       || e->e_ident[EI_DATA] != ELFDATA2MSB) {
      return ERR_ELF_WRONG;
   }

   len = 0;

   /*
    * AndreiW: This logic needs some review...
    */
   p = (Elf32_Phdr *) (load_buf + e->e_phoff);
   for (i = 0; i < e->e_phnum; ++i, ++p) {
      if (p->p_type != PT_LOAD || p->p_offset == 0)
         continue;
      if (len == 0) {
         off = p->p_offset;
         len = p->p_filesz;
         linked_base = p->p_vaddr & ADDRMASK;
      } else
         len = p->p_offset + p->p_filesz - off;
   }

   if (len == 0) {
      return ERR_ELF_NOT_LOADABLE;
   }

   entry = e->e_entry & ADDRMASK;
   if (len + off > load_buf_len) {
      len = load_buf_len - off;
   }

   image->buf = (vaddr_t) load_buf;
   image->linked_base = linked_base;
   image->text_offset = off;
   image->text_len = len;
   image->entry = entry;

   return ERR_NONE;
}


/*
 * Do any necessary relocations.
 *
 * This sounds loftier than what it really does,
 * which is hopefully absolutely nothing - new
 * kernels can run wherever they are loaded.
 */
quik_err_t elf_relo(boot_info_t *bi,
                    load_state_t *image)
{
   if (bi->flags & BOOT_PRE_2_4) {

      /*
       * 2.2 kernels have to execute at PA = 0x0 on the PowerMac.
       *
       * After this memmove, *p and *e may have been overwritten.
       * Entry points to kernel entry.
       */
      memmove((void *) image->linked_base,
              (void *) (image->buf + image->text_offset),
              image->text_len);
   } else {

      /*
       * Newer kernels can relocate themselves just fine, so we
       * always load them high - they'll probably not fit in the low region
       * anyway. Recompute entry and update apparent linked base.
       */
      image->entry -= image->linked_base;
      image->linked_base = image->buf + image->text_offset;
      image->entry += image->linked_base;
   }

   flush_cache(image->linked_base, image->text_len);

   return ERR_NONE;
}


/*
 * Given a loaded image hand off control to it.
 */
quik_err_t elf_boot(boot_info_t *bi,
                    load_state_t *image,
                    vaddr_t initrd_base,
                    length_t initrd_len,
                    char *params)
{
   vaddr_t start;

   /*
    * For the sake of the Open Firmware XCOFF loader, the entry
    * point may actually be a procedure descriptor.
    */
   start = *(vaddr_t *) image->entry;

   /* new boot strategy - see head.S in the kernel for more info -- Cort */
   if (start == 0x60000000) {
      /* nop */

      start = image->linked_base;
   } else {
      /* not the new boot strategy, use old logic -- Cort */

      if (start < image->linked_base || start >= image->linked_base + image->text_len
          || ((vaddr_t *)image->entry)[2] != 0) {

         /* doesn't look like a procedure descriptor */
         start += image->entry;
      }
   }

   printk("Starting at %p\n", start);
   set_bootargs(params);
   (* (void (*)()) start)(initrd_base, initrd_len,
                          (bi->flags & SHIM_OF) ?
                          bi->prom_shim :
                          bi->prom_entry,
                          0, 0);

   return ERR_KERNEL_RETURNED;
}
