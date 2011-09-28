## Process this file with automake to produce Makefile.in

if OS_WIN32
mwindows = -mwindows
endif

AM_LDFLAGS = $(mwindows)

SUBDIRS = cursors

bin_PROGRAMS = webexport

bindir = $(GIMP_LIBDIR)/plug-ins

webexport_SOURCES = \
	plugin-intl.h		\
	webx_main.c		\
	webx_main.h		\
	webx_pipeline.c		\
	webx_pipeline.h		\
	webx_target.c		\
	webx_target.h		\
        webx_indexed_target.c   \
        webx_indexed_target.h   \
	webx_jpeg_target.c	\
	webx_jpeg_target.h	\
        webx_png8_target.c      \
        webx_png8_target.h      \
        webx_png24_target.c     \
        webx_png24_target.h     \
        webx_gif_target.c       \
        webx_gif_target.h       \
	webx_crop_widget.c	\
	webx_crop_widget.h	\
	webx_resize_widget.c	\
	webx_resize_widget.h	\
	webx_dialog.c		\
	webx_dialog.h		\
	webx_prefs.c		\
	webx_prefs.h		\
	webx_preview.c		\
	webx_preview.h		\
	cursors.c		\
	cursors.h		\
	webx_utils.c		\
	webx_utils.h

AM_CPPFLAGS = \
	-DLOCALEDIR=\""$(LOCALEDIR)"\"		\
	-DDATADIR=\""$(DATADIR)"\"


INCLUDES = \
	-I$(top_srcdir)	\
	@GIMP_CFLAGS@	\
	$(GTK_CFLAGS)	\
	-I$(includedir)

LDADD = \
	$(GIMP_LIBS)		\
	$(GTK_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(LIBM)
