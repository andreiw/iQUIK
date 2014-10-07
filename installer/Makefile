all: iquik

COPTS = -O -Wall -Werror
CFLAGS = -I../include $(COPTS)

iquik: iquik.c
	$(CC) $(CFLAGS) -o iquik iquik.c

clean:
	rm -f *.o *~ iquik
