AM_CFLAGS = -I../.. -I$(srcdir)/../.. -Wall
EXTRA_DIST = .indent.pro 

noinst_LTLIBRARIES = libtskimg.la
libtskimg_la_SOURCES = img_open.c img_types.c raw.c raw.h \
    split.c split.h aff.c aff.h ewf.c ewf.h tsk_img_i.h img_io.c qemu_img.h qemu_img.c

indent:
	indent *.c *.h

clean-local:
	-rm -f *.c~ *.h~
