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
                   MainWindow.h \
                   MainWindow.cpp

$(OSMScout_OBJECTS) : moc_DBThread.cpp \
                      moc_MapWidget.cpp \
                      moc_SearchLocationDialog.cpp \
                      moc_RoutingDialog.cpp \
                      moc_MainWindow.cpp

OSMScout_CXXFLAGS=-I. \
                  $(LIBQTGUI_CFLAGS) \
                  $(LIBOSMSCOUT_CFLAGS) \
                  $(LIBOSMSCOUTMAP_CFLAGS)


OSMScout_LDADD= \
               $(LIBQTGUI_LIBS) \
               $(LIBOSMSCOUT_LIBS) \
               $(LIBOSMSCOUTMAP_LIBS)

if HAVE_LIB_QTOPENGL
OSMScout_CXXFLAGS += $(LIBQTOPENGL_CFLAGS)
OSMScout_LDADD += $(LIBQTOPENGL_LIBS)
endif

