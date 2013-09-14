/*
 * Definitions for talking to the Open Firmware PROM on
 * Power Macintosh computers.
 *
 * Copyright (C) 2013 Andrei Warkentin <andrey.warkentin@gmail.com>
 * Copyright (C) 1996 Paul Mackerras.
 */

#ifndef QUIK_PROM_H
#define QUIK_PROM_H

typedef void *phandle;
typedef void *ihandle;

extern ihandle prom_stdin;
extern ihandle prom_stdout;
extern ihandle prom_chosen;
extern ihandle prom_aliases;

/* Prototypes */
void prom_init(void (*pp)(void *), boot_info_t *bi);
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
void prom_pause(char *message);
void *prom_claim_chunk(void *virt,
                       unsigned int size,
                       unsigned int align);
void *prom_claim(void *virt, unsigned int size, unsigned int align);
void set_bootargs(char *params);

struct prom_args {
   char *service;
   int nargs;
   int nret;
   void *args[10];
};

#endif /* QUIK_PROM_H */
