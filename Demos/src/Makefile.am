AM_CXXFLAGS=$(LIBOSMSCOUT_CFLAGS) \
            $(LIBOSMSCOUTMAP_CFLAGS) \
            $(LIBCAIRO_CFLAGS) \
            $(LIBQTGUI_CFLAGS)
            
LDADD=$(LIBOSMSCOUT_LIBS) \
      $(LIBOSMSCOUTMAP_LIBS) \
      $(LIBCAIRO_LIBS) \
      $(LIBQTGUI_LIBS)

bin_PROGRAMS = Routing

if HAVE_LIB_CAIRO
bin_PROGRAMS += DrawMapCairo
endif

if HAVE_LIB_QTGUI
bin_PROGRAMS += DrawMapQt
endif

DrawMapCairo_SOURCES = DrawMapCairo.cpp

DrawMapQt_SOURCES = DrawMapQt.cpp

Routing_SOURCES = Routing.cpp

