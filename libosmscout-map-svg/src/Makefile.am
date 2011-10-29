AM_CPPFLAGS = $(LIB_CXXFLAGS) -DOSMSCOUTMAPSVGDLL -I$(top_srcdir)/include \
              $(LIBOSMSCOUT_CFLAGS) \
              $(LIBOSMSCOUTMAP_CFLAGS) 

lib_LTLIBRARIES = libosmscoutmapsvg.la
libosmscoutmapsvg_la_LDFLAGS  = $(LIBOSMSCOUT_LIBS) \
                                $(LIBOSMSCOUTMAP_LIBS)

libosmscoutmapsvg_la_SOURCES = osmscout/MapPainterSVG.cpp
