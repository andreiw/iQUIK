/*
 * ELF definitions.
 *
 * Kindly borrowed from the Linux kernel.
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

#ifndef QUIK_ELF_H
#define QUIK_ELF_H

/* 32-bit ELF base types. */
typedef uint32_t Elf32_Addr;
typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Off;
typedef int32_t  Elf32_Sword;
typedef uint32_t Elf32_Word;

#define EI_NIDENT 16

typedef struct elf32_hdr{
   unsigned char   e_ident[EI_NIDENT];
   Elf32_Half   e_type;
   Elf32_Half   e_machine;
   Elf32_Word   e_version;
   Elf32_Addr   e_entry;  /* Entry point */
   Elf32_Off e_phoff;
   Elf32_Off e_shoff;
   Elf32_Word   e_flags;
   Elf32_Half   e_ehsize;
   Elf32_Half   e_phentsize;
   Elf32_Half   e_phnum;
   Elf32_Half   e_shentsize;
   Elf32_Half   e_shnum;
   Elf32_Half   e_shstrndx;
} Elf32_Ehdr;

/* These constants define the permissions on sections in the program
   header, p_flags. */
#define PF_R      0x4
#define PF_W      0x2
#define PF_X      0x1

typedef struct elf32_phdr{
   Elf32_Word   p_type;
   Elf32_Off p_offset;
   Elf32_Addr   p_vaddr;
   Elf32_Addr   p_paddr;
   Elf32_Word   p_filesz;
   Elf32_Word   p_memsz;
   Elf32_Word   p_flags;
   Elf32_Word   p_align;
} Elf32_Phdr;

#define  EI_MAG0     0     /* e_ident[] indexes */
#define  EI_MAG1     1
#define  EI_MAG2     2
#define  EI_MAG3     3
#define  EI_CLASS    4
#define  EI_DATA     5

#define  ELFMAG0     0x7f     /* EI_MAG */
#define  ELFMAG1     'E'
#define  ELFMAG2     'L'
#define  ELFMAG3     'F'
#define  ELFCLASS32  1
#define  ELFDATA2MSB 2

/* These constants are for the segment types stored in the image headers */
#define PT_NULL    0
#define PT_LOAD    1

#endif
