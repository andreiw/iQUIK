/*
 * quik - bootblock installation program for Linux on Power Macintosh.
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

/* This program generates a lists of blocks where the secondary loader lives */
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

typedef unsigned u32;
typedef unsigned char u8;
#include <linux/fs.h>
#include <linux/ext2_fs.h>
#include <sys/stat.h>
#include <endian.h>
#include <fcntl.h>
#include <dirent.h>
#include <asm/mac-part.h>
#include <layout.h>

#define DFL_CONFIG      "/etc/quik.conf"
#define ALT_CONFIG      "/etc/milo.conf"
#define DFL_BACKUP      "/boot/old.b"
#define DFL_PRIMARY     "/boot/first.b"
#define DFL_SECONDARY   "/boot/second.b"

#define SD_MAJOR        8       /* Major device no. for scsi disks */
#define HDA_MAJOR       3       /* major number for hda and hdb */
#define HDC_MAJOR       22
#define HDE_MAJOR       33
#define HDG_MAJOR       34

#ifndef MAJOR
#define MAJOR(dev)      ((dev) >> 8)
#define MINOR(dev)      ((dev) & 0xff)
#endif

int unit_shift;
int part_mask;
#define UNIT(dev)       (((int)MINOR(dev)) >> unit_shift)
#define PART(dev)       (((int)MINOR(dev)) &  part_mask)

#define swab_32(x)      ((((x) >> 24) & 0xff) + (((x) >> 8) & 0xff00)   \
                         + (((x) & 0xff00) << 8) + (((x) & 0xff) << 24))
#define swab_16(x)      ((((x) >> 8) & 0xff) + (((x) & 0xff) << 8))

unsigned nsect;                 /* # (512-byte) sectors per f.s. block */
unsigned bs;                    /* f.s. block size */
int secsize;                    /* disk sector size */
int first_bootable;             /* part map entry # of 1st bootable part */
unsigned long doff;             /* start of partition containing 2nd boot */
char *first, *second, *old;     /* File names */
struct first_info finfo;
int verbose;

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

int check_fs(int fd)
{
   struct ext2_super_block sb; /* Super Block Info */

   if (lseek(fd, 1024, 0) != 1024
       || read(fd, &sb, sizeof(sb)) != sizeof (sb)) {
      fatal("Cannot read superblock");
   }

   if (sb.s_magic == EXT2_SUPER_MAGIC) {
      return 1024 << sb.s_log_block_size;
   }

   if (swab_16(sb.s_magic) == EXT2_SUPER_MAGIC) {
      return 1024 << swab_32(sb.s_log_block_size);
   }

   return -1;
}

void read_sb(char *device,    /* IN */
             char *bootdev  , /* IN */
             int floppy,      /* IN */
             int *part_index) /* OUT - blk# of partition map entry */
{
   int fd, partno, part;
   int offset, upart, maxpart;
   struct mac_partition *mp;
   struct mac_driver_desc *md;
   char *p;
   char buff[512];

   if (!floppy) {
      if ((fd = open(device, O_RDONLY)) == -1) {
         fatal("Cannot open %s", device);
      }

      bs = check_fs(fd);

      if (bs == -1) {
         fatal("Filesystems other than ext2 are not supported");
      }

      close(fd);
   }

   if (floppy) {

      /* First (and only) floppy on the disk. */
      partno = 2;
      bootdev = "/dev/fd0";
   } else {
      p = device + strlen(device);
      while (p > device && isdigit(p[-1])) {
         --p;
      }

      partno = atoi(p);
      if (partno == 0) {
         fatal("Can't determine partition number for %s", device);
      }
   }

   upart = 0;
   mp = (struct mac_partition *) buff;
   md = (struct mac_driver_desc *) buff;
   if ((fd = open(bootdev, O_RDONLY)) == -1) {
      fatal("Can't open %s", bootdev);
   }
   if (read(fd, buff, sizeof (buff)) != sizeof (buff)) {
      fatal("Error reading %s (block 0)", bootdev);
   }
   if (md->signature != MAC_DRIVER_MAGIC) {
      fatal("%s is not a mac-formatted disk", bootdev);
   }

   secsize = md->block_size;
   maxpart = 1;
   doff = 0;
   for (part = 1; part <= maxpart; ++part) {
      lseek(fd, part * secsize, 0);
      if (read(fd, buff, sizeof (buff)) != sizeof (buff)) {
         fatal("Error reading partition map from %s", bootdev);
      }

      if (mp->signature != MAC_PARTITION_MAGIC) {
         break;
      }

      if (part == 1) {
         maxpart = mp->map_count;
      }

      else if (maxpart != mp->map_count) {
         break;
      }

      if (first_bootable == 0 && (mp->status & STATUS_BOOTABLE) != 0
          && strcasecmp(mp->processor, "PowerPC") == 0) {
         first_bootable = part;
      }

      if (++upart == partno) {

         /* This is the one we want */
         *part_index = part;
         doff = mp->start_block * (secsize >> 9);
         break;
      }
   }

   if (upart < partno) {
      fatal("Couldn't locate partition %d on %s", partno, bootdev);
   }
}

void make_bootable(char *device,
                   char *spart,
                   int part_index,
                   off_t stage_size,
                   __u32 load_base)
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
      printf("Making %s bootable (map entry %d)\n", spart, part_index);
   }

   if (first_bootable > 0 && first_bootable < part_index) {
      fprintf(stderr, "Warning: prior partition (entry %d) is bootable\n",
              first_bootable);
   }

   if ((fd = open(device, O_RDWR)) < 0) {
      fatal("Cannot open %s for writing\n", device);
   }

   lseek(fd, part_index * secsize, 0);
   if (read(fd, buff, sizeof(buff)) != sizeof(buff)) {
      fatal("Error reading partition map entry %d from %s\n", part_index,
            device);
   }

   mp = (struct mac_partition *) buff;
   mp->status |= STATUS_BOOTABLE;
   mp->boot_start = 0;
   mp->boot_size = stage_size;
   mp->boot_load = load_base;
   mp->boot_load2 = 0;
   mp->boot_entry = load_base;
   mp->boot_entry2 = 0;
   strncpy(mp->processor, "PowerPC", sizeof(mp->processor));
   if (lseek(fd, part_index * secsize, 0) < 0
       || write(fd, buff, sizeof(buff)) != sizeof(buff)) {
      fatal("Couldn't make %s%d bootable: write error\n", device,
            part_index);
   }
}

int get_partition_blocks(char *filename)
{
   int fd;
   int block, i, k;
   struct stat st;

   if ((fd = open(filename, O_RDONLY)) == -1) {
      fatal("Cannot find %s", filename);
   }

   if (fstat(fd, &st) < 0) {
      fatal("Couldn't stat %s", filename);
   }

   finfo.blocksize = bs;
   nsect = bs >> 9;
   for (i = 0;; i++) {
      block = i;
      if (i * bs >= st.st_size || ioctl(fd, FIBMAP, &block) < 0 || !block) {
         break;
      }

      finfo.blknos[i] = block * nsect + doff;
   }

   finfo.nblocks = i;
   close(fd);
   return 0;
}

char *new_root = NULL;

char *chrootcpy(char *path)
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

void write_block_table(char *device, char *config_file, int partno)
{
   int fd;

   if (verbose) {
      printf("Writing block table to boot block on %s\n", device);
   }

   strncpy(finfo.quik_vers, "QUIK" VERSION, sizeof(finfo.quik_vers));
   finfo.second_base = SECOND_BASE;
   finfo.conf_part = partno;
   strncpy(finfo.conf_file, config_file, sizeof(finfo.conf_file));
   if ((fd = open(device, O_RDWR)) == -1) {
      fatal("Cannot open %s", device);
   }

   if (lseek(fd, FIRST_INFO_OFF, SEEK_SET) != FIRST_INFO_OFF) {
      fatal("Seek error on %s", device);
   }

   if (write(fd, &finfo, sizeof(finfo)) != sizeof(finfo)) {
      fatal("Couldn't update boot block on %s", device);
   }

   close(fd);
}

void usage(char *s)
{
   printf("QUIK " VERSION " Disk bootstrap installer for Powermac/Linux\n"
          "Usage: %s [options]\n"
          "Options:\n"
          " -r root_path chroots into root_path (all paths relative to this)\n"
          " -b secondary use secondary as second stage boot instead of /boot/second.b\n"
          " -i primary   install primary as first stage boot, instead of /boot/first.b\n"
          " -C config    specify alternate config file instead of /etc/quik.conf\n"
          "              (the config file has to reside on the same physical disk as\n"
          "              the second stage loader, but can be in a different partition)\n"
          " -s backup    save your old bootblock only if backup doesn't exist yet\n"
          " -S backup    force saving your old bootblock into backup\n"
          " -f           force overwriting of bootblock, even if quik already installed\n"
          " -F           install first stage to floppy"
          " -v           verbose mode\n"
          " -V           show version\n" ,s);
   exit (1);
}

int examine_bootblock(char *device, char *filename, int do_backup, int device_is_part)
{
   union {
      char buffer[1024];
      int first_word;
      struct {
         char pad[FIRST_INFO_OFF];
         struct first_info fi;
      } fi;
   } u;
   int fd, rc;
   FILE *fp;
   off_t off;
   int ret = 0;

   if ((fd = open(device, O_RDONLY)) == -1) {
      fatal("Cannot open %s", device);
   }

   if (device_is_part) {
      off = doff * 512;
   } else {
      off = 0;
   }

   if (lseek(fd, off, 0) != off) {
      fatal("Couldn't seek to partition start");
   }

   if (read (fd, &u, sizeof(u)) != sizeof(u)) {
      fatal("Couldn't read old bootblock");
   }

   close(fd);
   if (u.first_word != 0
       && memcmp(u.fi.fi.quik_vers, "QUIK" VERSION, 7) == 0
       && u.fi.fi.quik_vers[6] >= '0' && u.fi.fi.quik_vers[6] <= '9') {
      if (verbose) {
         printf("%s already has QUIK boot block installed\n", device);
         ret = 1;
      }
   }

   if (do_backup) {
      if (verbose)
         printf("Copying old bootblock from %s to %s\n", device, filename);
      if ((fp = fopen(filename, "w")) == NULL)
         fatal("Cannot create %s", filename);
      if (rc = fwrite(&u, 1, sizeof(u), fp) != sizeof(u))
         fatal("Couldn't write old bootblock to %s", filename);
      fclose (fp);
   }

   return ret;
}

void install_stage(char *device, char *filename, off_t *stage_size, int device_is_part)
{
   char *buff;
   int rc;
   int fd;
   FILE *fp;
   off_t off;
   struct stat st;

   if (verbose) {
      printf("Writing first-stage QUIK boot block to %s\n", device);
   }

   if ((fd = open(device, O_WRONLY)) == -1) {
      fatal("Couldn't open device %s for writing", device);
   }

   if ((fp = fopen(filename, "r")) == NULL) {
      fatal("Couldn't open primary boot file %s", filename);
   }

   if (stat(filename, &st) < 0) {
      fatal("Couldn't stat %s\n", filename);
   }
   *stage_size = st.st_size;

   buff = malloc(*stage_size);
   if (buff == NULL) {
      fatal("Couldn't alloc %u to read %s\n", *stage_size, filename);
   }

   rc = fread(buff, 1, *stage_size, fp);
   if (rc <= 0) {
      fatal("Couldn't read new quik bootblock from %s", filename);
   }

   if (device_is_part) {
      off = doff * 512;
   } else {
      off = 0;
   }

   if (lseek(fd, off, 0) != off) {
      fatal("Couldn't seek on %s", device);
   }

   if (write(fd, buff, rc) != rc) {
      fatal("Couldn't write quik bootblock to %s", device);
   }

   free(buff);
   close(fd);
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
   while (dir = readdir(dp)) {
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
         fatal("Couldn't stat %s\n", fn);
      }

      if (st3.st_dev == dev) {
         *q = c;
         return q;
      }

      if (S_ISLNK(st3.st_mode)) {
         len = readlink(buffer, readlinkbuf, 2048);
         if (len < 0) {
            fatal ("Couldn't readlink %s\n", fn);
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
            fatal("Internal error\n");
         }
      }
   }
}

int main(int argc,char **argv)
{
   char *name = NULL;
   char *install = NULL;
   char *first_dev_override = NULL;
   char *secondary;
   char *backup;
   char *config_file;
   int c;
   int version = 0;
   struct stat st1, st2;
   int fd;
   int force_backup = 0;
   int config_file_partno = 1;
   int part_index = 0;
   int part_start_block;
   char *p, *basedev;
   char bootdev[1024];
   char spart[1024];
   char buffer[1024];
   int force = 0;
   int f, inc_name;
   extern int optind;
   extern char *optarg;
   int floppy = 0;
   off_t stage_size = 0;

   /*
    * Test if we're being run on a chrp machine.  We don't
    * need to be run on a chrp machine so print a message
    * to this effect and exit gracefully.
    *  -- Cort
    */
   {
      FILE *f;
      char s[256];
      if ((f = fopen("/proc/cpuinfo","r")) == NULL)
      {
         fprintf(stderr,"Could not open /proc/cpuinfo!\n");
         exit(1);
      }

      while (!feof(f))
      {
         fgets(s, 256, f);
         if (!strncmp(s,"machine\t\t: CHRP", 15))
         {
            fprintf(stderr, "Quik does not need to be run "
                    "on CHRP machines.\n");
            exit(0);
         }
      }
   }

   config_file = NULL;
   backup = DFL_BACKUP;
   secondary = DFL_SECONDARY;
   new_root = NULL;
   name = argv[0];
   while ((c = getopt(argc, argv, "b:i:fC:S:s:r:FvVh")) != -1) {
      switch(c) {
      case 'b':
         secondary = optarg;
         break;
      case 'i':
         install = optarg;
         break;
      case 'f':
         force = 1;
         break;
      case 'C':
         config_file = optarg;
         break;
      case 'S':
         backup = optarg;
         force_backup = 1;
         break;
      case 's':
         backup = optarg;
         break;
      case 'r':
         new_root = optarg;
         break;
      case 'v':
         verbose = 1;
         break;
      case 'V':
         version = 1;
         break;
      case 'h':
         usage(name);
         break;
      case 'F':
         floppy = 1;
         break;
      }
   }

   if (version) {
      printf("QUIK version " VERSION "\n");
      exit(0);
   }

   if (optind < argc) {
      usage(name);
   }

   if (!new_root) {
      new_root = getenv("ROOT");
   }

   secondary = chrootcpy(secondary);
   if (stat(secondary, &st1) < 0) {
      fatal("Cannot open second stage loader %s", secondary);
   }

   if (!floppy) {

   /* work out what sort of disk this is and how to
      interpret the minor number */
   unit_shift = 6;
   part_mask = 0x3f;
   inc_name = 1;
   switch (MAJOR(st1.st_dev)) {
   case SD_MAJOR:
      unit_shift = 4;
      part_mask = 0x0f;
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
      p = find_dev(st1.st_dev);
      if (p == NULL) {
         fatal("Couldn't find out what device second stage boot is on");
      }

      basedev = p;
      unit_shift = 0;
      part_mask = 0;
      inc_name = 0;
   }

   strcpy(bootdev, basedev);
   if (inc_name) {
      bootdev[7] += UNIT(st1.st_dev);
   }

   strcpy(spart, bootdev);
   if (PART(st1.st_dev) != 0) {
      sprintf(spart+8, "%d", PART(st1.st_dev));
   }

   if (verbose) {
      printf("Second-stage loader is on %s\n", spart);
   }

   backup = chrootcpy (backup);

   if (config_file == NULL) {
      config_file = chrootcpy(DFL_CONFIG);
      if (stat(config_file, &st2) < 0) {
         char *p = chrootcpy(ALT_CONFIG);
         if (stat(p, &st2) == 0) {
            if (verbose) {
               printf("Using alternate config file %s\n", ALT_CONFIG);
            }
            config_file = p;
         }
      }
   } else {
      config_file = chrootcpy(config_file);
   }

   if (stat(config_file, &st2) >= 0) {
      if (MAJOR(st2.st_dev) != MAJOR(st1.st_dev)
          || UNIT(st2.st_dev) != UNIT(st1.st_dev)
          || PART(st2.st_dev) != PART(st1.st_dev)) {
         fatal("Config file %s has to be on the %s device (any partition)",
               config_file, bootdev);
      }
      strcpy(buffer, config_file);
      config_file = resolve_to_dev(buffer, st2.st_dev);
      if (inc_name) {
         config_file_partno = PART(st2.st_dev);
      } else {
         config_file_partno = 1;
      }

      if (verbose) {
         printf("Config file is on partition %d\n", config_file_partno);
      }
   }
   if (backup && !force_backup) {
      if (stat(backup, &st2) < 0) {
         force_backup = 1;
      }
   }
   } else {
      strcpy(bootdev, "/dev/fd0");
   }

   read_sb(spart, bootdev, floppy, &part_index);

   if (!floppy) {
      if (!examine_bootblock(spart, backup, force_backup, 0)
          || install || force) {
         if (!install) {
            install = chrootcpy(DFL_PRIMARY);
         } else if (*install == '/') {
            install = chrootcpy(install);
         }

         install_stage(spart, install, &stage_size, 0);
         make_bootable(bootdev,
                       spart,
                       part_index,
                       stage_size,
                       FIRST_BASE);
      }
   } else {
      install_stage("/dev/fd0", secondary, &stage_size, 1);
      make_bootable("/dev/fd0",
                    "/dev/fd0",
                    part_index,
                    stage_size,
                    SECOND_BASE);
   }

   if (!floppy) {
      get_partition_blocks(secondary);
      write_block_table(spart, config_file,
                        config_file_partno);
   }

   sync();
   exit(0);
}
