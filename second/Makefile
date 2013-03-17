##
## Linux Loader for Macintosh
##

CFLAGS = -I../include -O2 -D__NO_STRING_INLINES -nostdlib -fno-builtin
ASFLAGS=-D__ASSEMBLY__ -I../include
LDFLAGS=-T ld.script -N -Ttext 0x3e0000

OBJS =	crt0.o elf.o printf.o malloc.o main.o cmdline.o disk.o file.o \
	cfg.o strtol.o prom.o cache.o string.o setjmp.o ctype.o \
	nosys.o 

all: second.b

second: $(OBJS)
	$(LD) $(LDFLAGS) -Bstatic -o second $(OBJS) -lext2fs

crt0.o:        crt0.S
main.o:        main.c quik.h
malloc.o:      malloc.c quik.h
printf.o:      printf.c quik.h
cmdline.o:     cmdline.c quik.h
disk.o:	       disk.c quik.h
file.o:	       file.c quik.h
cfg.o:	       cfg.c quik.h

clean:
	$(RM) *.o second second.b core *~

second.b: second
	../util/elfextract second second.b
