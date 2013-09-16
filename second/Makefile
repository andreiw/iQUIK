##
## Linux Loader for Macintosh
##

CFLAGS = -I../include -O2 -D__NO_STRING_INLINES -nostdlib -fno-builtin -Werror -Wall
ASFLAGS=-D__ASSEMBLY__ -I../include
LDFLAGS=-T ld.script -N -Ttext 0x3e0000

all: second.b

OBJ = crt0.o elf.o printf.o malloc.o main.o cmdline.o disk.o file.o \
      cfg.o prom.o cache.o string.o setjmp.o util.o part.o \
      ext2fs.o

%.o: %.c
	$(CC) -c -MMD $(CFLAGS) $<
	@cp -f $*.d $*.d.tmp
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

%.o: %.S
	$(CC) -c -MMD $(ASFLAGS) $<
	@cp -f $*.d $*.d.tmp
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

-include $(OBJ:.o=.d)

second: $(OBJ)
	$(LD) $(LDFLAGS) -Bstatic -o second $^

clean:
	$(RM) *.o second second.b core *~ *.d

second.b: second
	../util/elfextract second second.b
