AM_CPPFLAGS = $(LIB_CXXFLAGS) -DOSMSCOUTMAPDLL -I$(top_srcdir)/include \
              $(LIBOSMSCOUT_CFLAGS) \
              $(LIBCAIRO_CFLAGS) \
              $(LIBQTGUI_CFLAGS) \
              $(LIBFREETYPE_CFLAGS) \
              $(LIBAGG_CFLAGS) \
              $(LIBPNG_CFLAGS)

lib_LTLIBRARIES = libosmscoutmap.la
libosmscoutmap_la_LDFLAGS  = $(LIBOSMSCOUT_LIBS) \
                             $(LIBCAIRO_LIBS) \
                             $(LIBQTGUI_LIBS) \
                             $(LIBFREETYPE_LIBS) \
                             $(LIBAGG_LIBS) \
                             $(LIBPNG_LIBS)

libosmscoutmap_la_SOURCES = osmscout/LoaderPNG.cpp \
                            osmscout/MapPainter.cpp

if HAVE_LIB_AGG
libosmscoutmap_la_SOURCES += osmscout/MapPainterAgg.cpp
endif

if HAVE_LIB_CAIRO
libosmscoutmap_la_SOURCES += osmscout/MapPainterCairo.cpp
endif

if HAVE_LIB_QTGUI
libosmscoutmap_la_SOURCES += osmscout/MapPainterQt.cpp
endif

if HAVE_DRIVER_SVG
libosmscoutmap_la_SOURCES += osmscout/MapPainterSVG.cpp
endif

