##
## Linux Loader for Macintosh
##

ENV_FILE=old_build_env
-include $(ENV_FILE)

ifeq ($(CONFIG_TINY), 1)
BUILD_FLAGS = -DCONFIG_TINY
endif
BUILD_ENV = "OLD_BUILD_FLAGS=$(BUILD_FLAGS)"

ifneq ($(BUILD_FLAGS),$(OLD_BUILD_FLAGS))
	PRETARGET=clean_env
else
	PRETARGET=log_env
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

all: $(PRETARGET) $(NAME).b

OBJ = crt0.o elf.o printf.o malloc.o main.o cmdline.o disk.o file.o \
      cfg.o prom.o cache.o string.o setjmp.o util.o part.o \
      crtsavres.o ext2fs.o env.o

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

log_env:
	@echo Resuming build with env \"$(BUILD_FLAGS)\"

log_clean_env:
	@echo Cleaning due to env change \(was \"$(OLD_BUILD_FLAGS)\" now \"$(BUILD_FLAGS)\"\)

clean_env: log_clean_env clean
	@echo $(BUILD_ENV) > $(ENV_FILE)

clean:
	@rm -f $(ENV_FILE) *.o $(NAME).elf $(NAME).b core *~ *.d

%.b: %.elf
	../util/elfextract $< $@
