AM_CPPFLAGS = -DOSMSCOUTCLIENTQTDLL -I$(top_srcdir)/include \
              $(LIBOSMSCOUT_CFLAGS) \
              $(LIBQT_CFLAGS)

lib_LTLIBRARIES = libosmscoutclientqt.la
libosmscoutclientqt_la_LDFLAGS  = -no-undefined \
                               $(LIBOSMSCOUT_LIBS) \
                               $(LIBQT_LIBS)

libosmscoutclientqt_la_SOURCES = osmscout/DBThread.cpp \
                                 osmscout/moc_DBThread.cpp \
                                 osmscout/MapWidget.cpp \
                                 osmscout/moc_MapWidget.cpp \
                                 osmscout/RoutingModel.cpp \
                                 osmscout/moc_RoutingModel.cpp \
                                 osmscout/LocationEntry.cpp \
                                 osmscout/moc_LocationEntry.cpp \
                                 osmscout/SearchLocationModel.cpp \
                                 osmscout/moc_SearchLocationModel.cpp \
                                 osmscout/Settings.cpp
osmscout/moc_%.cpp: $(top_srcdir)/include/osmscout/%.h
	@MOC@ -o$@ $<

CLEANFILES = osmscout/moc_DBThread.cpp \
             osmscout/moc_MapWidget.cpp \
             osmscout/moc_RoutingModel.cpp \
	     osmscout/moc_LocationEntry.cpp \
             osmscout/moc_SearchLocationModel.cpp
