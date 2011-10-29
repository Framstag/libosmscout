AM_CPPFLAGS = $(LIB_CXXFLAGS) -DOSMSCOUTMAPQTDLL -I$(top_srcdir)/include \
              $(LIBOSMSCOUT_CFLAGS) \
              $(LIBOSMSCOUTMAP_CFLAGS) \
              $(LIBQTGUI_CFLAGS)

lib_LTLIBRARIES = libosmscoutmapqt.la
libosmscoutmapqt_la_LDFLAGS  = $(LIBOSMSCOUT_LIBS) \
                               $(LIBOSMSCOUTMAP_LIBS) \
                               $(LIBQTGUI_LIBS)

libosmscoutmapqt_la_SOURCES = osmscout/MapPainterQt.cpp
