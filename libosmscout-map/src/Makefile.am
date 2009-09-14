AM_CPPFLAGS = $(LIB_CXXFLAGS) -DOSMSCDLL -I$(top_srcdir)/include \
              $(OSMSCOUT_CFLAGS) $(CAIRO_CFLAGS)

AM_LDFLAGS  = $(OSMSCOUT_LIBS) $(CAIRO_LIBS)

lib_LTLIBRARIES = libosmscoutmap.la
libosmscoutmap_la_SOURCES= osmscout/MapPainter.cpp
