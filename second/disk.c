#include "quik.h"
#include "disk.h"

quik_err_t
disk_open(char *device,
          ihandle *dev)
{
   ihandle current_dev;
   current_dev = call_prom("open", 1, 1, device);
   if (current_dev == (ihandle) 0 || current_dev == (ihandle) -1) {
      return ERR_DEV_OPEN;
   }

   *dev = current_dev;
   return ERR_NONE;
}


void
disk_close(ihandle dev)
{
   call_prom("close", 1, 1, dev);
}


void
disk_init(boot_info_t *bi)
{
   char *p;
   ihandle dev;
   quik_err_t err;

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

   err = disk_open(bi->bootdevice, &dev);
   if (err != ERR_NONE) {
      printk("Couldn't open '%s': %r\n", err);
      bi->bootdevice = NULL;
   }

   disk_close(dev);
}


length_t
disk_read(ihandle dev,
          char *buf,
          length_t nbytes,
          offset_t offset)
{
   length_t nr;

   if (nbytes == 0) {
      return 0;
   }

   nr = (length_t) call_prom("seek", 3, 1, dev,
                             (unsigned int) (offset >> 32),
                             (unsigned int) (offset & 0xFFFFFFFF));

   nr = (length_t) call_prom("read", 3, 1, dev,
                             buf, nbytes);
   return nr;
}
