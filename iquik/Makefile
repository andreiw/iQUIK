##
## Linux Loader for Macintosh
##

CFLAGS = -I../include -O2 -D__NO_STRING_INLINES -nostdlib -fno-builtin -Werror -Wall
ASFLAGS=-D__ASSEMBLY__ -I../include

#
# -Ttext value must match ../include/layout.h.
#
LDFLAGS=-T ld.script -N -Ttext 0x3e0000
NAME = iquik
CC = $(CROSS)gcc
LD = $(CROSS)ld

all: $(NAME).b

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

$(NAME): $(OBJ)
	$(LD) $(LDFLAGS) -Bstatic -o $@ $^

clean:
	@rm -f *.o $(NAME) $(NAME).b core *~ *.d

$(NAME).b: $(NAME)
	../util/elfextract $< $@
