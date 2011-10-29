AM_CPPFLAGS = $(LIB_CXXFLAGS) -DOSMSCOUTMAPDLL -I$(top_srcdir)/include \
              $(LIBOSMSCOUT_CFLAGS) \
              $(LIBCAIRO_CFLAGS) \
              $(LIBPNG_CFLAGS)

lib_LTLIBRARIES = libosmscoutmap.la
libosmscoutmap_la_LDFLAGS  = $(LIBOSMSCOUT_LIBS) \
                             $(LIBCAIRO_LIBS) \
                             $(LIBPNG_LIBS)

libosmscoutmap_la_SOURCES = osmscout/MapPainter.cpp

if HAVE_LIB_PNG
libosmscoutmap_la_SOURCES += osmscout/LoaderPNG.cpp
endif

if HAVE_LIB_CAIRO
libosmscoutmap_la_SOURCES += osmscout/MapPainterCairo.cpp
endif

