ASFLAGS = -D__ASSEMBLY__ -I../include
LDFLAGS = -T ld.script -Ttext 3f4000

all:	first.b

first.b: first
	size first
#	../util/elfextract first first.b
	dd if=first of=first.b bs=1k count=1 skip=16

OBJS = first.o

first.o: ../include/layout.h

first: $(OBJS)
	ld -o first $(LDFLAGS) $(OBJS)

clean:
	rm -f *.o *.s *~ first first.b
