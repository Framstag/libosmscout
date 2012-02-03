AM_CPPFLAGS = $(LIB_CXXFLAGS) \
              -DOSMSCOUTMAPSVGDLL -I$(top_srcdir)/include \
              $(LIBOSMSCOUTMAP_CFLAGS) \
              $(LIBOSMSCOUT_CFLAGS)

lib_LTLIBRARIES = libosmscoutmapsvg.la
libosmscoutmapsvg_la_LDFLAGS  = $(LIBOSMSCOUTMAP_LIBS) \
                                $(LIBOSMSCOUT_LIBS) 

libosmscoutmapsvg_la_SOURCES = osmscout/MapPainterSVG.cpp
