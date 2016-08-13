AM_CPPFLAGS = -DOSMSCOUTCLIENTQTDLL -I$(top_srcdir)/include \
              $(LIBOSMSCOUT_CFLAGS) \
              $(LIBQT_CFLAGS)

lib_LTLIBRARIES = libosmscoutclientqt.la
libosmscoutclientqt_la_LDFLAGS  = -no-undefined \
                               $(LIBOSMSCOUT_LIBS) \
                               $(LIBQT_LIBS)

libosmscoutclientqt_la_SOURCES = osmscout/DBThread.cpp \
                                 osmscout/MapWidget.cpp \
                                 osmscout/RoutingModel.cpp \
                                 osmscout/SearchLocationModel.cpp \
                                 osmscout/Settings.cpp
