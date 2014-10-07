##
## Linux Loader for Macintosh
##

CFLAGS = -I../include -O2 -D__NO_STRING_INLINES -nostdlib -fno-builtin -Werror -Wall
ASFLAGS=-D__ASSEMBLY__ -I../include
LDFLAGS=-T ld.script -N -Ttext 0x0
NAME = preboot

all: $(NAME).b

OBJ = preboot.o 

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
	$(RM) *.o $(NAME) $(NAME).b core *~ *.d

$(NAME).b: $(NAME)
	../util/elfextract $< $@
