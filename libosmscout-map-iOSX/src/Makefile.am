AM_CPPFLAGS = $(LIB_CXXFLAGS) \
              -I$(top_srcdir)/include \
              $(LIBOSMSCOUTMAP_CFLAGS) \
              $(LIBOSMSCOUT_CFLAGS)

lib_LTLIBRARIES = libosmscoutmapiosx.la
libosmscoutmapiosx_la_LDFLAGS  = -no-undefined \
                               $(LIBOSMSCOUTMAP_LIBS) \
                               $(LIBOSMSCOUT_LIBS)

libosmscoutmapiosx_la_SOURCES = osmscout/MapPainterIOS.mm
