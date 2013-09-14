/*
 * Global procedures and variables for the quik second-stage bootstrap.
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

#ifndef QUIK_QUIK_H
#define QUIK_QUIK_H

#include <inttypes.h>

#define ALIGN_UP(addr, align) (((addr) + (align) - 1) & (~((align) - 1)))
#define ALIGN(addr, align) (((addr) - 1) & (~((align) - 1)))
#define SIZE_1M 0x100000
#define NULL ((void *) 0)

typedef uint32_t vaddr_t;
typedef uint64_t offset_t;
typedef uint32_t length_t;

/*
 * Loaded kernels described by this code.
 */
typedef struct {
  vaddr_t buf;
  vaddr_t linked_base;
  vaddr_t text_offset;
  length_t text_len;
  vaddr_t  entry;
} load_state_t;

#define QUIK_ERR_LIST \
   QUIK_ERR_DEF(ERR_NONE, "no error")                                   \
   QUIK_ERR_DEF(ERR_DEV_OPEN, "cannot open device")                     \
   QUIK_ERR_DEF(ERR_DEV_SHORT_READ, "short read on device")             \
   QUIK_ERR_DEF(ERR_PART_NOT_MAC, "partitioning not macintosh")         \
   QUIK_ERR_DEF(ERR_PART_NOT_DOS, "partitioning not dos")               \
   QUIK_ERR_DEF(ERR_PART_NOT_PARTITIONED, "invalid disk partitioning")  \
   QUIK_ERR_DEF(ERR_PART_NOT_FOUND, "partition not found")              \
   QUIK_ERR_DEF(ERR_PART_BEYOND, "access beyond partition size")        \
   QUIK_ERR_DEF(ERR_FS_OPEN, "cannot open volume as file system")       \
   QUIK_ERR_DEF(ERR_FS_NOT_FOUND, "file not found")                     \
   QUIK_ERR_DEF(ERR_FS_NOT_EXT2, "FS is not ext2")                      \
   QUIK_ERR_DEF(ERR_FS_CORRUPT, "FS is corrupted")                      \
   QUIK_ERR_DEF(ERR_FS_LOOP, "symlink loop detected")                   \
   QUIK_ERR_DEF(ERR_FS_TOO_BIG, "file is too large to be loaded")       \
   QUIK_ERR_DEF(ERR_ELF_NOT, "invalid kernel image")                    \
   QUIK_ERR_DEF(ERR_ELF_WRONG, "invalid kernel architecture")           \
   QUIK_ERR_DEF(ERR_ELF_NOT_LOADABLE, "not a loadable image")           \
   QUIK_ERR_DEF(ERR_KERNEL_RETURNED, "kernel returned")                 \
   QUIK_ERR_DEF(ERR_NO_MEM, "malloc failed")                            \
   QUIK_ERR_DEF(ERR_INVALID, "invalid error, likely a bug")             \

#define QUIK_ERR_DEF(e, s) e,
typedef enum {
  QUIK_ERR_LIST
} quik_err_t;
#undef QUIK_ERR_DEF

typedef struct {

  /* Real OF entry. */
  vaddr_t prom_entry;

  /* Our shim, if any. */
  vaddr_t prom_shim;

  /* Boot device path. */
  char *device;

#define CONFIG_VALID          (1 << 1)
#define PAUSE_BEFORE_BOOT     (1 << 2)
#define DEBUG_BEFORE_BOOT     (1 << 3)
#define TRIED_AUTO            (1 << 4)
#define BOOT_PRE_2_4          (1 << 5)
#define SHIM_OF               (1 << 6)
  unsigned flags;

  /* Config file path. E.g. /etc/quik.conf */
  char *config_file;

  /*
   * Partition index for config_file, then
   * used as the default partition.
   */
  unsigned default_part;

  /* Picked default boot device. */
  char *default_device;

  /* Some reasonable size for bootargs. */
  char of_bootargs[512];
  char *bootargs;

  /* Pause message. */
  char *pause_message;

  /*
   * Initrd location, here because it might be needed by
   * PROM shimming.
   */
  void *initrd_base;
  length_t initrd_len;
} boot_info_t;

void cmdedit(void (*tabfunc)(boot_info_t *), boot_info_t *bi, int c);

extern char cbuff[];

int cfg_parse(char *cfg_file, char *buff, int len);
char *cfg_get_strg(char *image, char *item);
int cfg_get_flag(char *image, char *item);
void cfg_print_images(void);
char *cfg_get_default(void);

void *malloc(unsigned);
void spinner(int freq);

uint32_t swab32(uint32_t value);
uint16_t swab16(uint16_t value);
char *strstr(const char *s1, const char *s2);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, length_t n);
char *strcat(char *dest, const char *src);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, length_t n);
length_t strlen(const char *s);
char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);
void *memset(void *s, int c, length_t n);
void bcopy(const void *src, void *dest, length_t n);
void *memcpy(void *dest, const void *src, length_t n);
void *memmove(void *dest, const void *src, length_t n);
int memcmp(const void *s1, const void *s2, length_t n);
int tolower(int c);
int strcasecmp(const char *s1, const char *s2);
void * strdup(char *str);

#endif /* QUIK_QUIK_H */
