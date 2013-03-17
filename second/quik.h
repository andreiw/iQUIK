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

#include <inttypes.h>

#define ALIGN_UP(addr, align) (((addr) + (align) - 1) & (~((align) - 1)))
#define ALIGN(addr, align) (((addr) - 1) & (~((align) - 1)))
#define SIZE_1M 0x100000

typedef uint32_t vaddr_t;
typedef unsigned offset_t;
typedef unsigned length_t;

/*
 * Loaded kernels described by this code.
 */
typedef struct {
  vaddr_t buf;
  vaddr_t  linked_base;
  offset_t text_offset;
  length_t text_len;
  vaddr_t  entry;
} load_state_t;

#define QUIK_ERR_LIST \
   QUIK_ERR_DEF(ERR_NONE, "no error")                                  \
   QUIK_ERR_DEF(ERR_DEV_OPEN, "cannot open device")                    \
   QUIK_ERR_DEF(ERR_FS_OPEN, "cannot open volume as file system")      \
   QUIK_ERR_DEF(ERR_FS_NOT_FOUND, "file not found")                    \
   QUIK_ERR_DEF(ERR_FS_EXT2FS, "internal ext2fs error")                \
   QUIK_ERR_DEF(ERR_ELF_NOT, "invalid kernel image")                   \
   QUIK_ERR_DEF(ERR_ELF_WRONG, "invalid kernel architecture")          \
   QUIK_ERR_DEF(ERR_ELF_NOT_LOADABLE, "not a loadable image")          \
   QUIK_ERR_DEF(ERR_KERNEL_RETURNED, "kernel returned")                \
   QUIK_ERR_DEF(ERR_INVALID, "invalid error, likely a bug")            \

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
#define BOOT_FROM_SECTOR_ZERO (1 << 2)
#define PAUSE_BEFORE_BOOT     (1 << 3)
#define DEBUG_BEFORE_BOOT     (1 << 4)
#define TRIED_AUTO            (1 << 5)
#define BOOT_PRE_2_4          (1 << 6)
#define SHIM_OF               (1 << 7)
  unsigned flags;

  /* Config file path. E.g. /etc/quik.conf */
  char *config_file;

  /* Partition index for config_file. */
  unsigned config_part;

  /* Some reasonable size for bootargs. */
  char of_bootargs[512];
  char *bootargs;

  /* boot-device or /chosen/bootpath */
  char of_bootdevice[512];

  /* Picked default boot device. */
  char *bootdevice;

  /* Pause message. */
  char *pause_message;
} boot_info_t;

void diskinit(boot_info_t *bi);
void cmdedit(void (*tabfunc)(boot_info_t *), boot_info_t *bi, int c);

extern char cbuff[];

int cfg_parse(char *cfg_file, char *buff, int len);
char *cfg_get_strg(char *image, char *item);
int cfg_get_flag(char *image, char *item);
void cfg_print_images(void);
char *cfg_get_default(void);

void *malloc(unsigned);

