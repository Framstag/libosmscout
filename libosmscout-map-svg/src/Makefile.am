AM_CPPFLAGS = $(LIB_CXXFLAGS) \
              -DOSMSCOUTMAPSVGDLL -I$(top_srcdir)/include \
              $(LIBOSMSCOUTMAP_CFLAGS) \
              $(LIBOSMSCOUT_CFLAGS) \
              $(LIBPANGO_CFLAGS)

lib_LTLIBRARIES = libosmscoutmapsvg.la
libosmscoutmapsvg_la_LDFLAGS  = -no-undefined \
                                $(LIBOSMSCOUTMAP_LIBS) \
                                $(LIBOSMSCOUT_LIBS) \
                                $(LIBPANGO_LIBS) 

libosmscoutmapsvg_la_SOURCES = osmscout/MapPainterSVG.cpp
