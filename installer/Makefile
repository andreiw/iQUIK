all: iquik

COPTS = -O -Wall -Werror
CFLAGS = -I../include $(COPTS)

CC = $(CROSS)gcc

iquik: iquik.c
	$(CC) $(CFLAGS) -o iquik iquik.c

clean:
	rm -f *.o *~ iquik
