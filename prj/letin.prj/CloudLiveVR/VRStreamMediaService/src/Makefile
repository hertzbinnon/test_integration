include release/vrsmsz/Expiration
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

NANO_CFLAGS = $(shell pkg-config --cflags libcsoap)
NANO_LDFLAGS = $(shell pkg-config --libs libcsoap)

all: VRsmsz addstream test-effect-switch doc-effect-switch switch-src-element crossfade comp-switch-dynamic interpipe

clean:
	rm -f VRsmsz *.o addstream test-effect-switch doc-effect-switch switch-src-element crossfade comp-switch-dynamic


#httpd.o : httpd.c httpd.h
#	$(CC) -c  httpd.c $(CFLAGS) $(GST_CFLAGS) $(GSOUP_CFLAGS) $(LDFLAGS) $(GST_LDFLAGS)

httpd.o : httpd.c httpd.h vrsmsz.h
	$(CC) -c  httpd.c $(CFLAGS) $(GST_CFLAGS) $(NANO_CFLAGS) 

main.o : main.c httpd.h 
	$(CC) -c  main.c $(CFLAGS) $(GST_CFLAGS) 

vrsync.o : vrsync.c 
	$(CC) -c  vrsync.c -DYEAR=$(YEAR) -DMONTH=$(MONTH) -DDAY=$(DAY)

vrsmsz.o : vrsmsz.c vrsmsz.h
	$(CC) -c  vrsmsz.c $(CFLAGS) $(GST_CFLAGS) $(GLIB_JSON_CFLAGS) 

VRsmsz  : httpd.o main.o vrsmsz.o vrsync.o
	$(CC) -o $@ $^ $(CFLAGS) $(GST_CFLAGS) $(GLIB_JSON_LDFLAGS) $(NANO_LDFLAGS) $(LDFLAGS) $(GST_LDFLAGS)

addstream : examples/addstream.c
	$(CC) -o $@ $^ $(CFLAGS) $(GST_CFLAGS) $(GSOUP_LDFLAGS) $(LDFLAGS) $(GST_LDFLAGS)

test-effect-switch: examples/test-effect-switch.c
	$(CC) -o $@ $^ $(CFLAGS) $(GST_CFLAGS) $(GSOUP_LDFLAGS) $(LDFLAGS) $(GST_LDFLAGS)

doc-effect-switch: examples/doc-effect-switch.c
	$(CC) -o $@ $^ $(CFLAGS) $(GST_CFLAGS) $(GSOUP_LDFLAGS) $(LDFLAGS) $(GST_LDFLAGS)

switch-src-element: examples/switch-src-element.c
	$(CC) -o $@ $^ $(CFLAGS) $(GST_CFLAGS) $(GSOUP_LDFLAGS) $(LDFLAGS) $(GST_LDFLAGS)

crossfade: examples/crossfade.c
	$(CC) -o $@ $^ $(CFLAGS) $(GST_CFLAGS) $(GSOUP_LDFLAGS) $(LDFLAGS) $(GST_LDFLAGS) $(GST_CONTROLLER_CFLAGS) $(GST_CONTROLLER_LDFLAGS)

comp-switch-dynamic: examples/comp-switch-dynamic.c
	$(CC) -o $@ $^ $(CFLAGS) $(GST_CFLAGS) $(GSOUP_LDFLAGS) $(LDFLAGS) $(GST_LDFLAGS) $(GST_CONTROLLER_CFLAGS) $(GST_CONTROLLER_LDFLAGS)

interpipe: examples/interpipe.c
	$(CC) -o $@ $^ $(CFLAGS) $(GST_CFLAGS) $(GLIB_JSON_LDFLAGS) $(NANO_LDFLAGS) $(LDFLAGS) $(GST_LDFLAGS)
