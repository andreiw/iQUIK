/*
 * Global procedures and variables for the quik second-stage bootstrap.
 */

extern char cbuff[];
extern char bootdevice[];

int cfg_parse(char *cfg_file, char *buff, int len);
char *cfg_get_strg(char *image, char *item);
int cfg_get_flag(char *image, char *item);
void cfg_print_images(void);
char *cfg_get_default(void);

void *malloc(unsigned);

void prom_pause(void);
