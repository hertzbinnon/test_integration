CFLAGS ?= -O2 -g -Wall
LDFLAGS ?=

GST_CFLAGS = $(shell pkg-config --cflags gstreamer-1.0)
GST_LDFLAGS = $(shell pkg-config --libs gstreamer-1.0)

GIO_CFLAGS = $(shell pkg-config --cflags gio-2.0)
GIO_LDFLAGS = $(shell pkg-config --libs gio-2.0)

GSOUP_CFLAGS=$(shell pkg-config  --cflags libsoup-2.4)
GSOUP_LDFLAGS=$(shell pkg-config  --libs libsoup-2.4)

all: VRsmsz addstream

clean:
	rm -f VRsmsz *.o


httpd.o : httpd.c httpd.h
	$(CC) -c  httpd.c $(CFLAGS) $(GST_CFLAGS) $(GSOUP_CFLAGS) $(LDFLAGS) $(GST_LDFLAGS)

main.o : main.c httpd.h
	$(CC) -c  main.c $(CFLAGS) $(GST_CFLAGS) $(LDFLAGS) $(GST_LDFLAGS)

VRsmsz  : httpd.o main.o
	$(CC) -o $@ $^ $(CFLAGS) $(GST_CFLAGS) $(GSOUP_LDFLAGS) $(LDFLAGS) $(GST_LDFLAGS)

addstream : examples/addstream.c
	$(CC) -o $@ $^ $(CFLAGS) $(GST_CFLAGS) $(GSOUP_LDFLAGS) $(LDFLAGS) $(GST_LDFLAGS)

