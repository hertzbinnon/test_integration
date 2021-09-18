CFLAGS ?= -O2 -g -Wall
LDFLAGS ?=

GST_CFLAGS = $(shell pkg-config --cflags gstreamer-1.0)
GST_LDFLAGS = $(shell pkg-config --libs gstreamer-1.0)

GIO_CFLAGS = $(shell pkg-config --cflags gio-2.0)
GIO_LDFLAGS = $(shell pkg-config --libs gio-2.0)

GSOUP_CFLAGS=$(shell pkg-config  --cflags libsoup-2.4)
GSOUP_LDFLAGS=$(shell pkg-config  --libs libsoup-2.4)

GST_CONTROLLER_CFLAGS = $(shell pkg-config --cflags gstreamer-controller-1.0)
GST_CONTROLLER_LDFLAGS = $(shell pkg-config --libs gstreamer-controller-1.0)

GLIB_JSON_CFLAGS = $(shell pkg-config --cflags json-glib-1.0)
GLIB_JSON_LDFLAGS = $(shell pkg-config --libs json-glib-1.0)

#NANO_CFLAGS = $(shell pkg-config --cflags libcsoap)
#NANO_LDFLAGS = $(shell pkg-config --libs libcsoap)

all: vrsmsz

clean:
	rm -f vrsmsz *.o

simple-httpd.o : simple-httpd.c httpd.h ../gst/gstvrsmsz.h
	$(CC) -c  simple-httpd.c $(CFLAGS) $(GST_CFLAGS) $(GSOUP_CFLAGS) -I../gst

main.o : main.c httpd.h 
	$(CC) -c  main.c $(CFLAGS) $(GST_CFLAGS) -I../gst

vrsmsz  : simple-httpd.o main.o 
	$(CC) -o $@ $^ -pthread -L/home/director/lib -ljson-glib-1.0 $(GSOUP_LDFLAGS) ../__builddir/gst/libgstreamer-1.0.so -lgobject-2.0 -lglib-2.0
