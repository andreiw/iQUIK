#include "quik.h"
#include "prom.h"
#include <string.h>

static char current_devname[512];
static ihandle current_dev;

int open(char *device)
{
   current_dev = call_prom("open", 1, 1, device);
   if (current_dev == (ihandle) 0 || current_dev == (ihandle) -1) {
      printk("\nCouldn't open %s\n", device);
      return -1;
   }
   strcpy(current_devname, device);
   return 0;
}

char *strstr(const char * s1,const char * s2)
{
   int l1, l2;

   l2 = strlen(s2);
   if (!l2)
      return (char *) s1;
   l1 = strlen(s1);
   while (l1 >= l2) {
      l1--;
      if (!memcmp(s1,s2,l2))
         return (char *) s1;
      s1++;
   }
   return NULL;
}

void diskinit(boot_info_t *bi)
{
   char *p;

   if (bi->flags & BOOT_FROM_SECTOR_ZERO) {
      bi->bootdevice = bi->bootargs;
      word_split(&bi->bootdevice, &bi->bootargs);

      if (!bi->bootdevice) {
         printk("No booting device passed as argument.\n");
         return;
      }
   } else {
      bi->bootdevice = NULL;

      prom_get_chosen("bootpath", bi->of_bootdevice, sizeof(bi->of_bootdevice));
      if (bi->of_bootdevice[0] == 0) {
         prom_get_options("boot-device", bi->of_bootdevice, sizeof(bi->of_bootdevice));
      }

      if (bi->of_bootdevice[0] != 0) {
         bi->bootdevice = bi->of_bootdevice;
      }

      if (!bi->bootdevice) {
         printk("Could not figure out booting device from either /chosen/bootpath or boot-device.\n");
         return;
      }
   }

   p = strchr(bi->bootdevice, ':');
   if (p != 0) {
      if (bi->flags & BOOT_FROM_SECTOR_ZERO) {
         bi->config_part = strtol(p + 1, NULL, 0);
      }

      *p++ = 0;
      if (bi->flags & BOOT_FROM_SECTOR_ZERO) {

         /* Are we given a config file path? */
         p = strchr(p, '/');
         if (p != 0) {
            bi->config_file = p;
         } else {
            bi->config_file = "/etc/quik.conf";
         }
      }
   }

   if ((bi->flags & BOOT_FROM_SECTOR_ZERO) &&
       bi->config_part == 0) {

      /* No idea where's we looking for the configuration file. */
      printk("Booting device did not specify partition\n");
      return;
   }

   if (open(bi->bootdevice)) {
      printk("Couldn't open '%s' - bad parameter or changed hardware.\n");
      bi->bootdevice = NULL;
   }
}


int read(char *buf, int nbytes, long long offset)
{
   int nr;

   if (nbytes == 0)
      return 0;
   nr = (int)call_prom("seek", 3, 1, current_dev, (unsigned int) (offset >> 32),
                       (unsigned int) (offset & 0xFFFFFFFF));
   nr = (int) call_prom("read", 3, 1, current_dev, buf, nbytes);
   return nr;
}

void close()
{
   printk("Closing 0x%x\n", current_dev);
   call_prom("close", 1, 1, current_dev);
}

int setdisk(char *device)
{
   if (strcmp(device, current_devname) == 0)
      return 0;

   close();
   return open(device);
}
