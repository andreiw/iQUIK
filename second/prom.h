/*
 * Definitions for talking to the Open Firmware PROM on
 * Power Macintosh computers.
 *
 * Copyright (C) 1996 Paul Mackerras.
 */

typedef void *phandle;
typedef void *ihandle;

extern ihandle prom_stdin;
extern ihandle prom_stdout;
extern ihandle prom_chosen;
extern ihandle prom_aliases;

/* Prototypes */
void prom_init(void (*pp)(void *));
void prom_exit(void);
void *call_prom(char *service, int nargs, int nret, ...);
void prom_print(char *msg);
int putchar(int c);
int getchar(void);
int nbgetchar(void);
void prom_get_chosen(char *name, char *buf, int buflen);
void prom_get_options(char *name, char *buf, int buflen);
void prom_map(unsigned char *addr, unsigned len);
int get_ms(void);
void prom_pause(void);
