AM_CPPFLAGS = -DOSMSCOUTMAPQTDLL -I$(top_srcdir)/include \
              $(LIBOSMSCOUT_CFLAGS) \
              $(LIBQT_CFLAGS)

lib_LTLIBRARIES = libosmscoutmapqt.la
libosmscoutmapqt_la_LDFLAGS  = -no-undefined \
                               $(LIBOSMSCOUT_LIBS) \
                               $(LIBQT_LIBS)

libosmscoutmapqt_la_SOURCES = osmscout/MapPainterQt.cpp \
                              osmscout/SimplifiedPath.cpp
