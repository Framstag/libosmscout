include ${top_srcdir}/moc.mk

bin_PROGRAMS = OSMScout

OSMScout_SOURCES = OSMScout.cpp \
                   DBThread.h \
                   DBThread.cpp \
                   MapWidget.h \
                   MapWidget.cpp \
                   CitySearchDialog.h \
                   CitySearchDialog.cpp \
                   MainWindow.h \
                   MainWindow.cpp

$(OSMScout_OBJECTS) : moc_DBThread.cpp \
                      moc_MapWidget.cpp \
                      moc_CitySearchDialog.cpp \
                      moc_MainWindow.cpp

OSMScout_CXXFLAGS=-I. \
                  $(LIBQTGUI_CFLAGS) \
                  $(LIBOSMSCOUT_CFLAGS) \
                  $(LIBOSMSCOUTMAP_CFLAGS)

OSMScout_LDADD=$(LIBQTGUI_LIBS) \
               $(LIBOSMSCOUT_LIBS) \
               $(LIBOSMSCOUTMAP_LIBS)
