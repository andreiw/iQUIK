/* Second stage boot loader
   
   Copyright (C) 1996 Paul Mackerras.

   Because this program is derived from the corresponding file in the
   silo-0.64 distribution, it is also

   Copyright (C) 1996 Pete A. Zaitcev
   		 1996 Maurizio Plaza
   		 1996 David S. Miller
   		 1996 Miguel de Icaza
   		 1996 Jakub Jelinek
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#undef BOOTINFO

#include "quik.h"
#include <string.h>
#define __KERNEL__
#include <linux/elf.h>
#include <layout.h>
#ifdef BOOTINFO
#include <asm/machdep.h>
#endif

#define TMP_BUF		((unsigned char *) 0x14000)
#define TMP_END		((unsigned char *) SECOND_BASE)
#define ADDRMASK	0x0fffffff

char quik_conf[40];
int quik_conf_part;
unsigned int is_chrp = 0;

extern int start;
extern char bootdevice[];

int useconf = 0;
static int pause_after;
static char *pause_message = "Type go<return> to continue.\n";
static char given_bootargs[512];
static int given_bootargs_by_user = 0;

#define DEFAULT_TIMEOUT		-1

void fatal(const char *msg)
{
    printf("\nFatal error: %s\n", msg);
}

void maintabfunc (void)
{
    if (useconf) {
	cfg_print_images();
	printf("boot: %s", cbuff);
    }
}

void parse_name(char *imagename, int defpart, char **device,
		int *part, char **kname)
{
    int n;
    char *endp;

    /*
     * Assume partition 2 if no other has been explicitly set
     * in the config file on chrp -- Cort
     */
    if ( defpart == -1 )
	    defpart = 2;

    *kname = strchr(imagename, ':');
    if (!*kname) {
	*kname = imagename;
	*device = 0;
    } else {
	**kname = 0;
	(*kname)++;
	*device = imagename;
    }
    n = strtol(*kname, &endp, 0);
    if (endp != *kname) {
	*part = n;
	*kname = endp;
    } else if (defpart != -1)
	*part = defpart;
    else
	*part = 0;
    /* Range */
    if (**kname == '[')
	return;
    /* Path */
    if (**kname != '/')
	*kname = 0;
}

void
word_split(char **linep, char **paramsp)
{
    char *p;

    *paramsp = "\0";
    p = *linep;
    if (p == 0)
	return;
    while (*p == ' ')
	++p;
    if (*p == 0) {
	*linep = 0;
	return;
    }
    *linep = p;
    while (*p != 0 && *p != ' ')
	++p;
    while (*p == ' ')
	*p++ = 0;
    if (*p != 0)
	*paramsp = p;
}

char *
make_params(char *label, char *params)
{
    char *p, *q;
    static char buffer[2048];

    q = buffer;
    *q = 0;

    p = cfg_get_strg(label, "literal");
    if (p) {
	strcpy(q, p);
	q = strchr(q, 0);
	if (*params) {
	    if (*p)
		*q++ = ' ';
	    strcpy(q, params);
	}
	return buffer;
    }

    p = cfg_get_strg(label, "root");
    if (p) {
	strcpy (q, "root=");
	strcpy (q + 5, p);
	q = strchr (q, 0);
	*q++ = ' ';
    }
    if (cfg_get_flag(label, "read-only")) {
	strcpy (q, "ro ");
	q += 3;
    }
    if (cfg_get_flag(label, "read-write")) {
	strcpy (q, "rw ");
	q += 3;
    }
    p = cfg_get_strg(label, "ramdisk");
    if (p) {
	strcpy (q, "ramdisk=");
	strcpy (q + 8, p);
	q = strchr (q, 0);
	*q++ = ' ';
    }
    p = cfg_get_strg (label, "append");
    if (p) {
	strcpy (q, p);
	q = strchr (q, 0);
	*q++ = ' ';
    }
    *q = 0;
    pause_after = cfg_get_flag (label, "pause-after");
    p = cfg_get_strg(label, "pause-message");
    if (p)
	pause_message = p;
    if (*params)
	strcpy(q, params);

    return buffer;
}

int get_params(char **device, int *part, char **kname, char **params)
{
    int defpart = -1;
    char *defdevice = 0;
    char *p, *q, *endp;
    int c, n;
    char *imagename = 0, *label;
    int timeout = -1;
    int beg = 0, end;
    static int first = 1;
    static char bootargs[512];

    pause_after = 0;
    *params = "";

    if (first) {
	first = 0;
	prom_get_chosen("bootargs", bootargs, sizeof(bootargs));
	imagename = bootargs;
	word_split(&imagename, params);
	/* CHRP has garbage here -- Cort */
	if ( is_chrp )
		imagename[0] = 0;
	timeout = DEFAULT_TIMEOUT;
	if (useconf && (q = cfg_get_strg(0, "timeout")) != 0 && *q != 0)
	    timeout = strtol(q, NULL, 0);
    }
    printf("boot: ");
    c = -1;
    if (timeout != -1) {
	beg = get_ms();
	if (timeout > 0) {
	    end = beg + 100 * timeout;
	    do {
		c = nbgetchar();
	    } while (c == -1 && get_ms() <= end);
	}
	if (c == -1)
	    c = '\n';
    }

    if (c == '\n') {
	printf("%s", imagename);
	if (*params)
	    printf(" %s", *params);
	printf("\n");
    } else {
	cmdinit();
	cmdedit(maintabfunc, c);
	printf("\n");
	strcpy(given_bootargs, cbuff);
	given_bootargs_by_user = 1;
	imagename = cbuff;
	word_split(&imagename, params);
    }

    /* chrp gets this wrong, force it -- Cort */
    if ( useconf && (imagename[0] == 0) )
	imagename = cfg_get_default();

    label = 0;
    defdevice = bootdevice;

    if (useconf) {
	defdevice = cfg_get_strg(0, "device");
	p = cfg_get_strg(0, "partition");
	if (p) {
	    n = strtol(p, &endp, 10);
	    if (endp != p && *endp == 0)
		defpart = n;
	}
	p = cfg_get_strg(0, "pause-message");
	if (p)
	    pause_message = p;
	p = cfg_get_strg(imagename, "image");
	if (p && *p) {
	    label = imagename;
	    imagename = p;
	    defdevice = cfg_get_strg(label, "device");
	    p = cfg_get_strg(label, "partition");
	    if (p) {
		n = strtol(p, &endp, 10);
		if (endp != p && *endp == 0)
		    defpart = n;
	    }
	    *params = make_params(label, *params);
	}
    }

    if (!strcmp (imagename, "halt")) {
	prom_pause();
	*kname = 0;
	return 0;
    }

    if (imagename[0] == '$') {
	/* forth command string */
	call_prom("interpret", 1, 1, imagename+1);
	*kname = 0;
	return 0;
    }

    parse_name(imagename, defpart, device, part, kname);
    if (!*device)
	*device = defdevice;
    if (!*kname)
	    printf(
"Enter the kernel image name as [device:][partno]/path, where partno is a\n"
"number from 0 to 16.  Instead of /path you can type [mm-nn] to specify a\n"
"range of disk blocks (512B)\n");

    return 0;
}

/*
 * Print the specified message file.
 */
static void
print_message_file(char *p)
{
    char *q, *endp;
    int len = 0;
    int n, defpart = -1;
    char *device, *kname;
    int part;

    q = cfg_get_strg(0, "partition");
    if (q) {
	n = strtol(q, &endp, 10);
	if (endp != q && *endp == 0)
	    defpart = n;
    }
    parse_name(p, defpart, &device, &part, &kname);
    if (kname) {
	if (!device)
	    device = cfg_get_strg(0, "device");
	if (load_file(device, part, kname, TMP_BUF, TMP_END, &len, 1, 0)) {
	    TMP_BUF[len] = 0;
	    printf("\n%s", (char *)TMP_BUF);
	}
    }
}

/* Here we are launched */
int main(void *prom_entry, struct first_info *fip, unsigned long id)
{
    unsigned off;
    int i, len, image_len;
    char *kname, *params, *device;
    int part;
    int isfile, fileok = 0;
    unsigned int ret_offset;
    Elf32_Ehdr *e;
    Elf32_Phdr *p;
    unsigned load_loc, entry, start;
    extern char __bss_start, _end;
    struct first_info real_fip;
#ifdef BOOTINFO
    struct boot_info binf;
#endif

    /*
     * If we're not being called by the first stage bootloader
     * and we don't have the BootX signature assume we're
     * chrp. -- Cort
     */
    if ( (id != 0xdeadbeef) && ((unsigned long)prom_entry != 0x426f6f58) )
    {
	    is_chrp = 1;
	    prom_entry = (void *)id;	/* chrp passes prom_entry in r5 */
	    /*
	     * Make our own information packet with some default values
	     * we can assume are true on chrp.
	     */
	    fip = &real_fip;
	    strcpy( fip->quik_vers, "chrp" );
	    /*
	     * Assume root partition is partition 2.  We should
	     * scan the disk looking for a linux FS with /etc/quik.conf.
	     * -- Cort
	     */
	    fip->conf_part = 6;
	    strcpy( fip->conf_file, "/etc/quik.conf" );
    }

    if ( (unsigned long)prom_entry == 0x426f6f58 )
    {
	    printf("BootX launched us\n");
	    prom_entry = (void *)id;
	    /*
	     * These should come from the bootx info.
	     */
	    fip = &real_fip;
	    strcpy( fip->quik_vers, "BootX" );
	    /* Assume root partition is partition 9 -- Cort */
	    fip->conf_part = 9;
	    strcpy( fip->conf_file, "/etc/quik.conf" );
    }
	    
    memset(&__bss_start, 0, &_end - &__bss_start);
    prom_init(prom_entry);
    printf("Second-stage QUIK loader\n");

    if (diskinit() == -1)
	prom_exit();

    quik_conf_part = fip->conf_part;
    strncpy(quik_conf, fip->conf_file, sizeof(fip->conf_file));
    if (*quik_conf && quik_conf_part >= 0) {
	int len;
	fileok = load_file(0, quik_conf_part, quik_conf,
			   TMP_BUF, TMP_END, &len, 1, 0);
	if (!fileok || (unsigned) len >= 65535)
	    printf("\nCouldn't load %s\n", quik_conf);
	else {
	    char *p;
	    if (cfg_parse(quik_conf, TMP_BUF, len) < 0)
	        printf ("Syntax error or read error in %s.\n", quik_conf);
	    useconf = 1;
	    p = cfg_get_strg(0, "init-code");
	    if (p)
		call_prom("interpret", 1, 1, p);
	    p = cfg_get_strg(0, "init-message");
	    if (p)
		printf("%s", p);
	    p = cfg_get_strg(0, "message");
	    if (p)
		print_message_file(p);
	}
    } else
	printf ("\n");

    for (;;) {
	get_params(&device, &part, &kname, &params);
	if (!kname)
	    continue;
	
	fileok = load_file(device, part, kname,
			   TMP_BUF, TMP_END, &image_len, 1, 0);

	if (!fileok) {
	    printf ("\nImage not found.... try again\n");
	    continue;
	}
	if (image_len > TMP_END - TMP_BUF) {
	    printf("\nImage is too large (%u > %u)\n", image_len,
		   TMP_END - TMP_BUF);
	    continue;
	}

	/* By this point the first sector is loaded (and the rest of */
	/* the kernel) so we check if it is an executable elf binary. */

	e = (Elf32_Ehdr *) TMP_BUF;
	if (!(e->e_ident[EI_MAG0] == ELFMAG0 &&
	      e->e_ident[EI_MAG1] == ELFMAG1 &&
	      e->e_ident[EI_MAG2] == ELFMAG2 &&
	      e->e_ident[EI_MAG3] == ELFMAG3)) {
	    printf ("\n%s: unknown image format\n", kname);
	    continue;
	}

	if (e->e_ident[EI_CLASS] != ELFCLASS32
	    || e->e_ident[EI_DATA] != ELFDATA2MSB) {
	    printf("Image is not a 32bit MSB ELF image\n");
	    continue;
	}
	len = 0;
	p = (Elf32_Phdr *) (TMP_BUF + e->e_phoff);
	for (i = 0; i < e->e_phnum; ++i, ++p) {
	    if (p->p_type != PT_LOAD || p->p_offset == 0)
		continue;
	    if (len == 0) {
		off = p->p_offset;
		len = p->p_filesz;
		load_loc = p->p_vaddr & ADDRMASK;
	    } else
		len = p->p_offset + p->p_filesz - off;
	}
	if (len == 0) {
	    printf("Cannot find a loadable segment in ELF image\n");
	    continue;
	}
	entry = e->e_entry & ADDRMASK;
	if (len + off > image_len)
	    len = image_len - off;
	break;
    }
    
    /* chrp expects to start at 0x10000 */
    if ( is_chrp )
	    load_loc = entry = 0x10000;
    /* After this memmove, *p and *e may have been overwritten. */
    memmove((void *)load_loc, TMP_BUF + off, len);
    flush_cache(load_loc, len);

    close();
    if (pause_after) {
        printf("%s", pause_message);
	prom_pause();
        printf("\n");
    }

    /*
     * For the sake of the Open Firmware XCOFF loader, the entry
     * point may actually be a procedure descriptor.
     */
    start = *(unsigned *)entry;
    /* new boot strategy - see head.S in the kernel for more info -- Cort */
    if (start == 0x60000000/* nop */ )
	    start = load_loc;
    /* not the new boot strategy, use old logic -- Cort */
    else
    {
	    if (start < load_loc || start >= load_loc + len
		|| ((unsigned *)entry)[2] != 0)
		    /* doesn't look like a procedure descriptor */
		    start += entry;
    }
    printf("Starting at %x\n", start);
#ifdef BOOTINFO    
    /* setup the bootinfo */
    binf.magic_start = bin.magic_end = BOOT_INFO_MAGIC;
    sprintf( binf.cmd_line, "%s", params );
    sprintf( binf.boot_loader, "Quik" );
    if ( is_chrp )
	    binf._machine = _MACH_chrp;
    else
	    binf._machine = _MACH_Pmac;
    binf.initrd_start = binf.initrd_size = 0;
    binf.systemmap_start = binf.systemmap_size = 0;
    /* put the boot info at the nearest 0x10000 boundry from the end -- Cort */
    memcpy( ((load_loc + len)+0x10000) & ~0x10000, binf,
	    sizeof(struct boot_info) );
#endif    
    (* (void (*)()) start)(params, 0, prom_entry, 0, 0);
    prom_exit();
}

