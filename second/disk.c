#include "quik.h"
#include "prom.h"
#include <string.h>

char bootdevice[512];
static char current_devname[512];
static ihandle current_dev;

int open(char *device)
{
   current_dev = call_prom("open", 1, 1, device);
   if (current_dev == (ihandle) 0 || current_dev == (ihandle) -1) {
      printf("\nCouldn't open %s\n", device);
      return -1;
   }
   strcpy(current_devname, device);
   return 0;
}

char * strstr(const char * s1,const char * s2)
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

int diskinit()
{
   char *p;
   extern unsigned int is_chrp;

   prom_get_chosen("bootpath", bootdevice, sizeof(bootdevice));
   if (bootdevice[0] == 0) {
      prom_get_options("boot-device", bootdevice, sizeof(bootdevice));
      if (bootdevice[0] == 0)
         fatal("Couldn't determine boot device");
   }
   p = strchr(bootdevice, ':');
   if (p != 0)
      *p = 0;
   /*
    * Hack for the time being.  We need at the raw disk device, not
    * just a few partitions. -- Cort
    */
   /*     if ( is_chrp ) */
   /*        sprintf(bootdevice, "disk:0"); */
   if( open(bootdevice) )
   {
      /*
       * Some chrp machines do not have a 'disk' alias so
       * try this if disk:0 fails
       *   -- Cort
       */
      sprintf(bootdevice, "/pci@fee00000/scsi@c/sd@8:0");
      return open(bootdevice);
   }
   return 0;
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
}

int setdisk(char *device)
{
   if (strcmp(device, current_devname) == 0)
      return 0;
   close();
   return open(device);
}
