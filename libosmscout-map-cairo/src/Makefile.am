AM_CPPFLAGS = $(LIB_CXXFLAGS) -DOSMSCOUTMAPCAIRODLL -I$(top_srcdir)/include \
              $(LIBOSMSCOUT_CFLAGS) \
              $(LIBOSMSCOUTMAP_CFLAGS) \
              $(LIBCAIRO_CFLAGS) \
              $(LIBPNG_CFLAGS)

lib_LTLIBRARIES = libosmscoutmapcairo.la
libosmscoutmapcairo_la_LDFLAGS  = $(LIBOSMSCOUT_LIBS) \
                                  $(LIBOSMSCOUTMAP_LIBS) \
                                  $(LIBCAIRO_LIBS) \
                                  $(LIBPNG_LIBS)

libosmscoutmapcairo_la_SOURCES = osmscout/MapPainterCairo.cpp

if HAVE_LIB_PNG
libosmscoutmapcairo_la_SOURCES += osmscout/LoaderPNG.cpp
endif


