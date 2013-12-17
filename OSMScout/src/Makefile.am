include ${top_srcdir}/moc.mk

bin_PROGRAMS = OSMScout

OSMScout_SOURCES = OSMScout.cpp \
                   DBThread.h \
                   DBThread.cpp \
                   MapWidget.h \
                   MapWidget.cpp \
                   SearchLocationDialog.h \
                   SearchLocationDialog.cpp \
                   RoutingDialog.h \
                   RoutingDialog.cpp \
                   Settings.h \
                   Settings.cpp \
                   SettingsDialog.h \
                   SettingsDialog.cpp \
                   MainWindow.h \
                   MainWindow.cpp

$(OSMScout_OBJECTS) : moc_DBThread.cpp \
                      moc_MapWidget.cpp \
                      moc_SearchLocationDialog.cpp \
                      moc_RoutingDialog.cpp \
                      moc_SettingsDialog.cpp \
                      moc_MainWindow.cpp

OSMScout_CXXFLAGS=-I. \
                  $(LIBOSMSCOUTMAPQT_CFLAGS) \
                  $(LIBOSMSCOUTMAP_CFLAGS) \
                  $(LIBOSMSCOUT_CFLAGS) \
                  $(LIBQTGUI_CFLAGS)


OSMScout_LDADD= \
               $(LIBOSMSCOUTMAPQT_LIBS) \
               $(LIBOSMSCOUTMAP_LIBS) \
               $(LIBOSMSCOUT_LIBS) \
               $(LIBQTGUI_LIBS)

