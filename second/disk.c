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

   /*
    * Out params is indeed '0' or the close won't happen. Grrr...
    */
   call_prom("close", 1, 0, dev);
}


void
disk_init(boot_info_t *bi)
{
   char *p;
   ihandle dev;
   quik_err_t err;

   bi->bootdevice = bi->bootargs;
   word_split(&bi->bootdevice, &bi->bootargs);

   if (!bi->bootdevice) {
      printk("No booting device passed as argument.\n");
      return;
   }

   p = strchr(bi->bootdevice, ':');
   if (p != 0) {
      bi->config_part = strtol(p + 1, NULL, 0);
      *p++ = 0;

      /* Are we given a config file path? */
      p = strchr(p, '/');
      if (p != 0) {
         bi->config_file = p;
      } else {
         bi->config_file = "/etc/quik.conf";
      }
   }

   if (bi->config_part == 0) {

      /* No idea where's we looking for the configuration file. */
      printk("Booting device did not specify partition\n");
      return;
   }
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
