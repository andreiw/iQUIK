/*
 * iquik - bootblock installation program for Linux on Power Macintosh.
 *
 * Copyright (C) 2013 Andrei Warkentin <andrey.warkentin@gmail.c>
 * Copyright (C) 1996 Paul Mackerras.
 *
 * Derived from silo.c in the silo-0.6.4 distribution, therefore:
 *
 * Copyright (C) 1996 Maurizio Plaza
 *               1996 Jakub Jelinek
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
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <layout.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <mac-part.h>
#include <layout.h>
#include <endian.h>
#include <stdbool.h>

#define DFL_BOOTBLOCK   "/boot/iquik.b"

#define SD_MAJOR        8       /* Major device no. for scsi disks */
#define HDA_MAJOR       3       /* major number for hda and hdb */
#define HDC_MAJOR       22
#define HDE_MAJOR       33
#define HDG_MAJOR       34

#ifndef MAJOR
#define MAJOR(dev)      ((dev) >> 8)
#endif

static bool verbose = false;
static bool test_mode = false;


ssize_t do_write(int fd, const void *buf, size_t count)
{
   if (test_mode) {
      printf("<skipping write due to test mode>\n");
      return count;
   }

   return write(fd, buf, count);
}

void fatal(char *fmt,...)
{
   va_list ap;
   va_start(ap, fmt);
   fprintf(stderr, "Fatal error: ");
   vfprintf(stderr, fmt, ap);
   putc('\n', stderr);
   va_end(ap);
   exit(1);
}


void read_sb(char *device,         /* IN */
             unsigned *part_index, /* OUT - # of boot partition entry  */
             off_t *doff,          /* OUT - block start for boot partition */
             ssize_t *secsize)     /* OUT - sector size. */
{
   int fd, part;
   int maxpart;
   struct mac_partition *mp;
   struct mac_driver_desc *md;
   char buff[512];

   mp = (struct mac_partition *) buff;
   md = (struct mac_driver_desc *) buff;
   if ((fd = open(device, O_RDONLY)) == -1) {
      fatal("Can't open '%s'", device);
   }

   if (read(fd, buff, sizeof (buff)) != sizeof (buff)) {
      fatal("Error reading '%s' (block 0)", device);
   }

   if (be16toh(md->signature) != MAC_DRIVER_MAGIC) {
      fatal("'%s' is not a mac-formatted disk", device);
   }

   *secsize = be16toh(md->block_size);
   maxpart = 1;
   *doff = 0;
   for (part = 1; part <= maxpart; ++part) {
      lseek(fd, part * *secsize, 0);
      if (read(fd, buff, sizeof (buff)) != sizeof (buff)) {
         fatal("Error reading partition map from '%s'", device);
      }

      if (be16toh(mp->signature) != MAC_PARTITION_MAGIC) {
         break;
      }

      if (part == 1) {
         maxpart = be32toh(mp->map_count);
      }

      else if (maxpart != be32toh(mp->map_count)) {
         break;
      }

      if (!strcmp(mp->type, APPLE_BOOT_TYPE)) {

         /* This is the one we want */
         *part_index = part;
         *doff = be32toh(mp->start_block) * (*secsize >> 9);
         return;
      }
   }

   fatal("No Apple_Bootstrap partition found on '%s'", device);
}


void make_bootable(char *device,
                   ssize_t secsize,
                   unsigned part_index,
                   ssize_t stage_size,
                   uint32_t load_base)
{
   int fd;
   struct mac_partition *mp;
   char buff[512];

   if (part_index == 0) {
      if (verbose) {
         printf("Not making bootable - no partition\n");
      }

      return;
   }

   if (verbose) {
      printf("Making '%s' bootable (map entry %d)\n", device, part_index);
   }

   if ((fd = open(device, O_RDWR)) < 0) {
      fatal("Cannot open '%s' for writing", device);
   }

   lseek(fd, part_index * secsize, 0);
   if (read(fd, buff, sizeof(buff)) != sizeof(buff)) {
      fatal("Error reading partition map entry %d from '%s'", part_index,
            device);
   }

   mp = (struct mac_partition *) buff;
   mp->status |= htobe32(STATUS_BOOTABLE);
   mp->boot_start = 0;
   mp->boot_size = htobe32(stage_size);
   mp->boot_load = htobe32(load_base);
   mp->boot_load2 = 0;
   mp->boot_entry = htobe32(load_base);
   mp->boot_entry2 = 0;
   strncpy(mp->processor, "PowerPC", sizeof(mp->processor));
   if (lseek(fd, part_index * secsize, 0) < 0) {
      fatal("Couldn't make '%s'%d bootable: seek error", device,
            part_index);
   }

   if (do_write(fd, buff, sizeof(buff)) != sizeof(buff)) {
      fatal("Couldn't make '%s'%d bootable: write error", device,
            part_index);
   }
}


char *chrootcpy(char *new_root, char *path)
{
   if (new_root && *new_root) {
      char *buffer = malloc(strlen(new_root) + strlen(path) + 1);
      strcpy(buffer, new_root);
      if (new_root[strlen(new_root) - 1] != '/') {
         strcat(buffer, path);
      } else {
         strcat(buffer, path + 1);
      }

      return buffer;
   } else {
      return strdup(path);
   }
}


void usage(char *s)
{
   printf("iQUIK " VERSION " Disk bootstrap installer for PowerMac/Linux\n"
          "Copyright (C) 2013 Andrei Warkentin <andrey.warkentin@gmail.com>\n"
          "Usage: '%s' [options]\n"
          "Options:\n"
          " -r root_path chroots into root_path (all paths relative to this)\n"
          " -b bblock    use bblock as boot code instead of /boot/iquik.b\n"
          " -d           install boot code to alternate device (e.g. /dev/fd0)\n"
          " -v           verbose mode\n"
          " -p script    preboot script\n"
          " -T           test mode (no actual writes)\n"
          " -V           show version\n" ,s);
   exit(1);
}


void install_stage(char *device,
                   char *filename,
                   char *preboot,
                   ssize_t *stage_size,
                   off_t doff)
{
   char *buff;
   int rc;
   int fd;
   FILE *fpp;
   FILE *fp;
   off_t off;
   ssize_t code_size;
   struct stat st;
   uint32_t magic;

   if (verbose) {
      printf("Writing iQUIK boot block to '%s'\n", device);
   }

   if ((fd = open(device, O_WRONLY)) == -1) {
      fatal("Couldn't open device '%s' for writing", device);
   }

   if ((fp = fopen(filename, "r")) == NULL) {
      fatal("Couldn't open primary boot file '%s'", filename);
   }

   if (stat(filename, &st) < 0) {
      fatal("Couldn't stat '%s'", filename);
   }

   code_size = st.st_size;

   if (preboot != NULL) {
      if ((fpp = fopen(preboot, "r")) == NULL) {
         fatal("Couldn't open preboot script '%s'", preboot);
      }

      if (stat(preboot, &st) < 0) {
         fatal("Couldn't stat '%s'", preboot);
      }

      code_size -= sizeof(PREBOOT_MAGIC);
      *stage_size = code_size + st.st_size + 1;
  } else {
      fpp = NULL;
      *stage_size = code_size;
   }

   if (verbose) {
      printf("Code size: %u\nStage size: %u bytes\n",
             code_size, *stage_size);
   }

   buff = malloc(*stage_size);
   if (buff == NULL) {
      fatal("Couldn't alloc %u to read '%s'", *stage_size, filename);
   }

   rc = fread(buff, 1, code_size, fp);
   if (rc <= 0) {
      fatal("Couldn't read iQUIK boot code from '%s'", filename);
   }

   if (preboot != NULL) {
      rc = fread(&magic, 1, sizeof(magic), fp);
      if (rc <= 0) {
         fatal("Couldn't read iQUIK boot code preboot signature from '%s'",
               filename);
      }

      if (be32toh(magic) != PREBOOT_MAGIC) {
         fatal("'%s' has bad preboot magic 0x%08x, preboot scripts unsupported\n",
               filename, magic);
      }

      rc = fread(buff + code_size, 1, *stage_size - code_size - 1, fpp);
      if (rc <= 0) {
         fatal("Couldn't read preboot script from '%s'", preboot);
      }


      buff[*stage_size - 1] = '\0';
   }

   off = doff * 512;
   if (verbose) {
     printf("Installing to offset %ju on '%s'\n", (uintmax_t) off, device);
   }

   if (lseek(fd, off, 0) != off) {
      fatal("Couldn't seek on '%s'", device);
   }

   if (do_write(fd, buff, *stage_size) != *stage_size) {
      fatal("Couldn't write iQUIK boot code to '%s'", device);
   }

   free(buff);
   close(fd);

   if (preboot != NULL) {
      fclose(fpp);
   }

   fclose(fp);
}


char *find_dev(int number)
{
#define DEVNAME "/dev"
   DIR *dp;
   char *p;
   struct dirent *dir;
   static char name[PATH_MAX+1];
   struct stat s;

   if (!number) {
      return NULL;
   }

   if ((dp = opendir(DEVNAME)) == NULL) {
      return NULL;
   }

   strcpy(name, DEVNAME "/");
   p = strchr(name, 0);
   while ((dir = readdir(dp)) != NULL) {
      strcpy(p, dir->d_name);

      if (stat(name, &s) < 0) {
         return NULL;
      }

      if (S_ISBLK(s.st_mode) && s.st_rdev == number) {
         return name;
      }
   }
   return NULL;
}


char *
resolve_to_dev(char *buffer, dev_t dev)
{
   char *p, *q, *b, *r;
   char *fn;
   int len, c;
   struct stat st3;
   char readlinkbuf[2048];
   char buffer2[2048];

   for (p = buffer;;) {
      q = strchr (p, '/');

      if (q) {
         c = *q;
         *q = 0;
      } else {
         c = 0;
      }

      fn = *buffer ? buffer : "/";
      if (lstat(fn, &st3) < 0) {
         fatal("Couldn't stat '%s'", fn);
      }

      if (st3.st_dev == dev) {
         *q = c;
         return q;
      }

      if (S_ISLNK(st3.st_mode)) {
         len = readlink(buffer, readlinkbuf, 2048);
         if (len < 0) {
            fatal("Couldn't readlink '%s'", fn);
         }

         readlinkbuf[len] = 0;
         if (*readlinkbuf == '/') {
            b = r = readlinkbuf;
         } else {
            b = buffer2;
            strcpy (b, buffer);
            r = strchr(b, 0);
            *r++ = '/';
            strcpy(r, readlinkbuf);
         }

         if (c) {
            r += len;
            if (r[-1] != '/') {
               *r++ = '/';
            }

            strcpy (r, q + 1);
         }

         strcpy (buffer, b);
         p = buffer + 1;
      } else {
         *q = c;
         p = q + 1;
         if (!c) {
            fatal("Internal error");
         }
      }
   }
}


int main(int argc,char **argv)
{
   char *new_root = NULL;
   char *basedev = NULL;
   char *name = DFL_BOOTBLOCK;
   char *preboot = NULL;
   int c;
   struct stat st1;
   int version = 0;
   off_t doff = 0;
   unsigned part_index = 0;
   ssize_t secsize = 0;
   ssize_t stage_size = 0;

   /*
    * Test if we're being run on a CHRP machine.  We don't
    * need to be run on a CHRP machine, as the later can
    * run the ELF directly.
    *  -- Cort
    */
   {
      FILE *f;
      char s[256];
      char *ps;

      if ((f = fopen("/proc/cpuinfo","r")) == NULL) {
         fatal("Could not open /proc/cpuinfo!\n");
      }

      while (!feof(f)) {
         ps = fgets(s, 256, f);
         if (ps != NULL &&
             !strncmp(ps, "machine\t\t: CHRP", 15)) {
            fatal("iQUIK must be installed differently on CHRP machines\n");
         }
      }
   }

   while ((c = getopt(argc, argv, "p:b:d:r:vVTh")) != -1) {
      switch(c) {
      case 'p':
         preboot = optarg;
         break;
      case 'b':
         name = optarg;
         break;
      case 'r':
         new_root = optarg;
         break;
      case 'v':
         verbose = 1;
         break;
      case 'T':
         test_mode = 1;
         break;
      case 'V':
         version = 1;
         break;
      case 'h':
         usage(argv[0]);
         break;
      case 'd':
         basedev = optarg;
         break;
      }
   }

   if (version) {
      printf("iQUIK version " VERSION "\n");
      exit(0);
   }

   if (optind < argc) {
      usage(argv[0]);
   }

   if (!new_root) {
      new_root = getenv("ROOT");
   }

   name = chrootcpy(new_root, name);
   if (stat(name, &st1) < 0) {
      fatal("Cannot open iQUIK boot block '%s'", name);
   }

   if (basedev == NULL) {

      /*
       * work out what sort of disk this is.
       */
      switch (MAJOR(st1.st_dev)) {
      case SD_MAJOR:
         basedev = "/dev/sda";
         break;
      case HDA_MAJOR:
         basedev = "/dev/hda";
         break;
      case HDC_MAJOR:
         basedev = "/dev/hdc";
         break;
      case HDE_MAJOR:
         basedev = "/dev/hde";
         break;
      case HDG_MAJOR:
         basedev = "/dev/hdg";
         break;
      default:
         basedev = find_dev(st1.st_dev);
         if (basedev == NULL) {
            fatal("Couldn't find out what device to install to (please use override)");
         }
      }
   } else {
      printf("Using overriden install device '%s'\n", basedev);
   }

   read_sb(basedev, &part_index, &doff, &secsize);
   install_stage(basedev, name, preboot, &stage_size, doff);
   make_bootable(basedev,
                 secsize,
                 part_index,
                 stage_size,
                 IQUIK_BASE);
   sync();
   exit(0);
}
