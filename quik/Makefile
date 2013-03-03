all:	quik

COPTS = -O
CFLAGS = -I../include $(COPTS)

quik: quik.c
	$(CC) $(CFLAGS) -o quik quik.c

clean:
	rm -f *.o *~ quik
