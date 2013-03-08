/*
 * File related stuff.
 *
 * Copyright (C) 1996 Maurizio Plaza
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

#include <ctype.h>
#define _STDLIB_H
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include "quik.h"
#include "file.h"

typedef unsigned __u32;
typedef unsigned u32;
typedef unsigned char u8;
#include <linux/ext2_fs.h>
#include <ext2fs/ext2fs.h>
#include "setjmp.h"
#include <asm/mac-part.h>

static errcode_t linux_open (const char *name, int flags, io_channel * channel);
static errcode_t linux_close (io_channel channel);
static errcode_t linux_set_blksize (io_channel channel, int blksize);
static errcode_t linux_read_blk (io_channel channel, unsigned long block, int count, void *data);
static errcode_t linux_write_blk (io_channel channel, unsigned long block, int count, const void *data);
static errcode_t linux_flush (io_channel channel);

/*
 * Some firmware trashes the first part of the data section -
 * this protects it.
 *   -- Cort
 */
char protect[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

static struct struct_io_manager struct_linux_manager =
{
   EXT2_ET_MAGIC_IO_MANAGER,
   "linux I/O Manager",
   linux_open,
   linux_close,
   linux_set_blksize,
   linux_read_blk,
   linux_write_blk,
   linux_flush
};

extern int read(char *, int, long long);

ext2_filsys fs = 0;
static ino_t root, cwd;
static io_manager linux_io_manager = &struct_linux_manager;

static unsigned int bs;    /* Blocksize */
static unsigned long long doff;  /* Byte offset where partition starts */
static int decompress;

static unsigned char *filebuffer;
static unsigned char *filelimit;
static unsigned long block_no;
static unsigned long lblock;
static int block_cnt;
static enum { ext2 } type;
static unsigned long size;

static int read_mac_partition(int part)
{
   int rc, upart, blocks_in_map, i;
   int secsize;
   char blk[512];
   struct mac_partition *mp = (struct mac_partition *) blk;
   struct mac_driver_desc *md = (struct mac_driver_desc *) blk;

   rc = read(blk, 512, 0LL);
   if (rc != 512) {
      fatal("Cannot read driver descriptor");
      return 0;
   }
   if (md->signature != MAC_DRIVER_MAGIC) {
      /*fatal("Not a macintosh-formatted disk");*/
      return 0;
   }
   secsize = md->block_size;
   blocks_in_map = 1;
   upart = 0;
   for (i = 1; i <= blocks_in_map; ++i) {
      if (read(blk, 512, (long long) i * secsize) != 512) {
         /*fatal("Error reading partition map");*/
         return 0;
      }
      if (mp->signature != MAC_PARTITION_MAGIC)
         break;
      if (i == 1)
         blocks_in_map = mp->map_count;
      ++upart;
      /* If part is 0, use the first bootable partition. */
      if (part == upart
          || (part == 0 && (mp->status & STATUS_BOOTABLE) != 0
              && strcasecmp(mp->processor, "powerpc") == 0)) {
         doff = (long long) mp->start_block * secsize;
         return 1;
      }
   }
   /*fatal("Partition %d not found", part);*/
   return 0;
}

unsigned long swab32(unsigned long value)
{
   __u32 result;

   __asm__("rlwimi %0,%1,24,16,23\n\t"
           "rlwimi %0,%1,8,8,15\n\t"
           "rlwimi %0,%1,24,0,7"
           : "=r" (result)
           : "r" (value), "0" (value >> 24));
   return result;
}

static int read_dos_partition(int part)
{
   int rc, i, v;
   char blk[512];
   struct partition {
      unsigned char boot_ind;   /* 0x80 - active */
      unsigned char head;    /* starting head */
      unsigned char sector;  /* starting sector */
      unsigned char cyl;     /* starting cylinder */
      unsigned char sys_ind; /* What partition type */
      unsigned char end_head;   /* end head */
      unsigned char end_sector; /* end sector */
      unsigned char end_cyl; /* end cylinder */
      unsigned int start_sect;  /* starting sector counting from 0 */
      unsigned int nr_sects; /* nr of sectors in partition */
   };
   struct partition *p;

   rc = read(blk, 512, 0LL);
   if (rc != 512) {
      fatal("Cannot read first block");
      return 0;
   }

   /* check the MSDOS partition magic */
   if ( (blk[0x1fe] != 0x55) || (blk[0x1ff] != 0xaa) )
   {
      printk("No MSDOS partition magic number on disk!\n");
      return 0;
   }
   if ( part >= 4 )
      fatal("msdos partition numbers > 4");
   p = (struct partition *)&blk[0x1be];
   for ( i = 1 ; i != part ; i++,p++)
      /* nothing */ ;
   doff = swab32(p->start_sect)*512;
   return 1;
}

int sprintk (char *buf, char *fmt,...)
{
   strcpy (buf, fmt);
}

void com_err (const char *a, long i, const char *fmt,...)
{
   printk ((char *) fmt);
}

static errcode_t linux_open (const char *name, int flags, io_channel * channel)
{
   int partno;
   io_channel io;

   if (!name)
      return EXT2_ET_BAD_DEVICE_NAME;
   io = (io_channel) malloc (sizeof (struct struct_io_channel));
   if (!io)
      return EXT2_ET_BAD_DEVICE_NAME;
   memset (io, 0, sizeof (struct struct_io_channel));
   io->magic = EXT2_ET_MAGIC_IO_CHANNEL;
   io->manager = linux_io_manager;
   io->name = (char *) malloc (strlen (name) + 1);
   strcpy (io->name, name);
   io->block_size = bs;
   io->read_error = 0;
   io->write_error = 0;

   doff = 0;
   partno = *(name + strlen (name) - 1) - '0';
   if (!read_mac_partition(partno) && !read_dos_partition(partno) )
      return EXT2_ET_BAD_DEVICE_NAME;

   *channel = io;
   return 0;
}

static errcode_t linux_close (io_channel channel)
{
}

static errcode_t linux_set_blksize (io_channel channel, int blksize)
{
   channel->block_size = bs = blksize;
}

static errcode_t linux_read_blk (io_channel channel, unsigned long block, int count, void *data)
{
   int size;
   long long tempb;

   tempb = (((long long) block) * ((long long)bs)) + doff;
   size = (count < 0) ? -count : count * bs;
   if (read (data, size, tempb) != size) {
      printk ("\nRead error on block %d", block);
      return EXT2_ET_SHORT_READ;
   }
   return 0;
}

static errcode_t linux_write_blk (io_channel channel, unsigned long block, int count, const void *data)
{
}

static errcode_t linux_flush (io_channel channel)
{
}

static int open_ext2 (char *device)
{
   int retval;

   retval = ext2fs_open (device, EXT2_FLAG_RW, 0, 0, linux_io_manager, &fs);
   if (retval)
      return 0;
   root = cwd = EXT2_ROOT_INO;
   return 1;
}

static void rotate (int freq)
{
   static int i = 0;
   static char rot[] = "\\|/-";

   if (!(i % freq))
      printk ("%c\b", rot[(i / freq) % 4]);
   i++;
}

static int dump_block(ext2_filsys fs, blk_t *blocknr, int lg_block,
                      void *private)
{
   size_t nbytes, nzero;

   if (lg_block < 0)
      return 0;

   rotate(5);

   if (block_no && block_cnt < 256
       && *blocknr == block_no + block_cnt
       && lg_block == lblock + block_cnt) {
      ++block_cnt;
      return 0;
   }
   if (block_no) {
      nbytes = block_cnt * bs;
      if (io_channel_read_blk(fs->io, block_no, block_cnt, filebuffer))
         return BLOCK_ABORT;
      filebuffer += nbytes;
      lblock += block_cnt;
      block_no = 0;
   }
   if (lg_block && lg_block != lblock) {
      nzero = (lg_block - lblock) * bs;
      memset(filebuffer, 0, nzero);
      filebuffer += nzero;
      lblock = lg_block;
   }
   if (*blocknr) {
      block_no = *blocknr;
      block_cnt = 1;
   }
   return 0;
}

static int dump_finish (void)
{
   if (block_no) {
      blk_t tmp = 0;
      if (dump_block(fs, &tmp, 0, 0))
         return 0;
   }
   if (lblock * bs < size)
      memset(filebuffer, 0, size - lblock * bs);
   return 1;
}

static int dump_ext2(ino_t inode, char *filename)
{
   errcode_t retval;

   block_no = lblock = 0;
   block_cnt = 0;
   retval = ext2fs_block_iterate(fs, inode, 0, 0, dump_block, 0);
   if (retval) {
      printk ("Error %d reading %s", retval, filename);
      return 0;
   }
   return dump_finish();
}

int dump_device_range(char *filename, char *bogusdev, int *len,
                      void (*lenfunc)(int, char **, char **))
{
   /* Range of blocks on physical block device */
   int start, end = -1;
   int retval;
   char *p;

   bs = 512;
   p = strchr (filename, '-');
   filename++;
   if (p && *filename >= '0' && *filename <= '9') {
      *p = 0;
      start = strtol(filename, NULL, 0);
      filename = p + 1;
      p = strchr(filename, ']');
      if (p && *filename >= '0' && *filename <= '9' && !p[1]) {
         *p = 0;
         end = strtol(filename, NULL, 0);
      }
   }
   if (end == -1) {
      printk ("\n"
              "Ranges of physical blocks are specified as {prom_path:}{partno}[xx-yy]\n"
              "where {} means optional part, partno defaults to 0 (i.e. whole disk)\n"
              "and xx is the starting block (chunk of 512 bytes) and yy end (not\n"
              "inclusive, i.e. yy won't be read)\n");
      return 0;
   }
   if (lenfunc)
      (*lenfunc)((end - start) << 9, (char **)&filebuffer, (char **)&filelimit);
   fs = (ext2_filsys) malloc (sizeof (struct struct_ext2_filsys));
   if (fs) {
      if (!linux_open(bogusdev, 0, &fs->io)) {
         blk_t tmp;

         block_no = 0;
         for (tmp = start; tmp < end; tmp++) {
            if (dump_block (fs, &tmp, tmp - start, 0))
               break;
         }

         if (tmp == end && dump_finish()) {
            if (len)
               *len = (end - start) << 9;
            return 1;
         }
      }
   }
   printk ("\nInternal error while loading blocks from device\n");
   return 0;
}

int get_len(ino_t inode)
{
   struct ext2_inode ei;

   if (ext2fs_read_inode (fs, inode, &ei))
      return 0;
   return ei.i_size;
}

quik_err_t length_file(char *device,
                       int partno,
                       char *filename,
                       fs_len_t *len)
{
   int retval;
   ext2_ino_t inode;
   struct ext2_inode ei;
   char bogusdev[] = "/dev/sdaX";

   *len = 0;

   if (setdisk(device) < 0) {
      return ERR_DEV_OPEN;
   }

   bogusdev[8] = partno + '0';
   if (!open_ext2(bogusdev)) {
      return ERR_FS_OPEN;
   }

   retval = ext2fs_namei(fs, root, cwd, filename, &inode);
   if (retval) {
      if (retval == EXT2_ET_FILE_NOT_FOUND) {
         return ERR_FS_NOT_FOUND;
      }

      printk("ext2fs error 0x%x while opening %s.", retval, filename);

      ext2fs_close(fs);
      return ERR_FS_EXT2FS;
   }

   if (ext2fs_read_inode(fs, inode, &ei)) {
      return ERR_FS_EXT2FS;
   }

   *len = ei.i_size;
   return ERR_NONE;
}

int load_file(char *device, int partno, char *filename,
              char *buffer, char *limit, int *len,
              int dogunzip, void (*lenfunc)(int, char **, char **))
{
   ext2_ino_t inode;
   int retval;
   char bogusdev[] = "/dev/sdaX";

   if (setdisk(device) < 0) {
      return 0;
   }

   bogusdev[8] = partno + '0';
   filebuffer = buffer;
   filelimit = limit;
   decompress = dogunzip & 1;
   if (*filename == '[')
      return dump_device_range (filename, bogusdev, len, lenfunc);
   if (!open_ext2(bogusdev)) {
      fatal ("Unable to open filesystem");
      return 0;
   }
   type = ext2;
   retval = ext2fs_namei(fs, root, cwd, filename, &inode);
   if (retval) {
      if (retval == EXT2_ET_FILE_NOT_FOUND) {
         printk("File '%s' does not exist.", filename);
      } else {
         printk("ext2fs error 0x%x while loading file %s.", retval, filename);
      }
      ext2fs_close(fs);
      return 0;
   }
   size = get_len(inode);
   if (buffer + size > limit) {
      printk("Image too large to fit in destination\n");
      return 0;
   }

   if (lenfunc)
      (*lenfunc)(size, (char **)&filebuffer, (char **)&filelimit);

   retval = 0;
   if (inode)
      retval = dump_ext2(inode, filename);

   if (retval && len)
      *len = size;
   ext2fs_close(fs);

   return retval;
}
