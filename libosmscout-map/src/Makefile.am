AM_CPPFLAGS = $(LIB_CXXFLAGS) -DOSMSCDLL -I$(top_srcdir)/include \
              $(LIBOSMSCOUT_CFLAGS) \
              $(LIBCAIRO_CFLAGS) \
              $(LIBPNG_CFLAGS)

AM_LDFLAGS  = $(LIBOSMSCOUT_LIBS) \
              $(LIBCAIRO_LIBS) \
              $(LIBPNG_LIBS)

lib_LTLIBRARIES = libosmscoutmap.la

libosmscoutmap_la_SOURCES = osmscout/LoaderPNG.cpp \
                            osmscout/MapPainter.cpp \
                            osmscout/MapPainterCairo.cpp 
