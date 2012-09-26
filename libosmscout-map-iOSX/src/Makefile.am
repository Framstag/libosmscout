AM_CPPFLAGS = $(LIB_CXXFLAGS) \
              -I$(top_srcdir)/include \
              $(LIBOSMSCOUTMAP_CFLAGS) \
              $(LIBOSMSCOUT_CFLAGS) \
              $(LIBQTGUI_CFLAGS)

lib_LTLIBRARIES = libosmscoutmapiosx.la
libosmscoutmapqt_la_LDFLAGS  = -no-undefined \
                               $(LIBOSMSCOUTMAP_LIBS) \
                               $(LIBOSMSCOUT_LIBS) \
                               $(LIBQTGUI_LIBS)

libosmscoutmapqt_la_SOURCES = osmscout/MapPainterIOS.cpp
