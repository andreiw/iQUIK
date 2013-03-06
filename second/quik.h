/*
 * Global procedures and variables for the quik second-stage bootstrap.
 */

typedef struct {

  /* Boot device path. */
  char *device;

#define CONFIG_VALID          (1 << 1)
#define BOOT_FROM_SECTOR_ZERO (1 << 2)
#define PAUSE_BEFORE_BOOT     (1 << 3)
#define DEBUG_BEFORE_BOOT     (1 << 4)
#define TRIED_AUTO            (1 << 5)
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

void prom_pause(void);
