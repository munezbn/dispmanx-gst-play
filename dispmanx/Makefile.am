bin_PROGRAMS = dispmanx-gst-play

dispmanx_gst_play_SOURCES = dispmanx-gst-play.c dispmanx-gst-play.h dispmanx-window.c gst-play-kb.c gst-play-kb.h

LDADD = $(GSTREAMER_LIBS) $(GLIB_LIBS) $(LIBM)  $(BCM_HOST_LIBS)

AM_CFLAGS = $(GSTREAMER_CFLAGS) $(GLIB_CFLAGS) $(WARNING_CFLAGS)  $(BCM_HOST_CFLAGS)

noinst_HEADERS = gst-play-kb.h
