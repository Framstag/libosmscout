AM_CXXFLAGS=$(LIBOSMSCOUT_CFLAGS) \
            $(LIBOSMSCOUTMAP_CFLAGS) \
            $(LIBCAIRO_CFLAGS) \
            $(LIBQTGUI_CFLAGS)
            
LDADD=$(LIBOSMSCOUT_LIBS) \
      $(LIBOSMSCOUTMAP_LIBS) \
      $(LIBCAIRO_LIBS) \
      $(LIBQTGUI_LIBS)

bin_PROGRAMS = AddressLookup Routing

if HAVE_LIB_CAIRO
bin_PROGRAMS += DrawMapCairo
endif

if HAVE_LIB_QTGUI
bin_PROGRAMS += DrawMapQt \
                ResourceConsumptionQt
endif

AddressLookup_SOURCES = AddressLookup.cpp

DrawMapCairo_SOURCES = DrawMapCairo.cpp

DrawMapQt_SOURCES = DrawMapQt.cpp

ResourceConsumptionQt_SOURCES = ResourceConsumptionQt.cpp

Routing_SOURCES = Routing.cpp

