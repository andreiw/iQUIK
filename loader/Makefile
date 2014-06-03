##
## Linux Loader for Macintosh
##

ifeq ($(CONFIG_TINY), 1)
BUILD_FLAGS = -DCONFIG_TINY
endif

CFLAGS = -I../include -Os -D__NO_STRING_INLINES -nostdlib -fno-builtin -Werror -Wall $(BUILD_FLAGS)
ASFLAGS=-D__ASSEMBLY__ -I../include $(BUILD_FLAGS)

#
# -Ttext value must match ../include/layout.h.
#
LDFLAGS=-T ld.script -N -Ttext 0x3e0000
NAME = iquik
CC = $(CROSS)gcc
LD = $(CROSS)ld

POSSIBLE_CLEAN=$(shell (echo '$(BUILD_FLAGS)' | cmp -s - build_flags && echo noclean) || ( echo '$(BUILD_FLAGS)' > build_flags && echo clean ))
all: $(POSSIBLE_CLEAN) $(NAME).b

OBJ = crt0.o elf.o printf.o malloc.o main.o cmdline.o disk.o file.o \
      cfg.o prom.o cache.o string.o setjmp.o util.o part.o \
      crtsavres.o ext2fs.o

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

$(NAME).elf: $(OBJ)
	$(LD) $(LDFLAGS) -Bstatic -o $@ $^

noclean:
	@echo No need to clean

clean:
	@rm -f *.o $(NAME).elf $(NAME).b core *~ *.d

%.b: %.elf
	../util/elfextract $< $@
