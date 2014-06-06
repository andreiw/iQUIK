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
quik_err_t prom_init(void (*pp)(void *));
void prom_exit(void);
void *call_prom(char *service, int nargs, int nret, ...);
void prom_print(char *msg);
int putchar(int c);
key_t getchar(void);
key_t nbgetchar(void);
void prom_get_chosen(char *name, char *buf, int buflen);
void prom_get_options(char *name, char *buf, int buflen);
void prom_map(unsigned char *addr, unsigned len);
int get_ms(void);
void prom_pause(char *message);
void prom_interpret(char *buf);
void prom_ensure_claimed(void *virt, unsigned int size);
void prom_release(void *virt, unsigned int size);
void *prom_claim_chunk(void *virt,
                       unsigned int size);
void *prom_claim(void *virt, unsigned int size);
quik_err_t prom_open(char *device, ihandle *ih);
void set_bootargs(char *params);

struct prom_args {
   char *service;
   int nargs;
   int nret;
   void *args[10];
};

#endif /* QUIK_PROM_H */
