AM_CXXFLAGS=$(LIBOSMSCOUT_CFLAGS) \
            $(LIBOSMSCOUTMAP_CFLAGS) \
            $(LIBCAIRO_CFLAGS) \
            $(LIBQTGUI_CFLAGS) \
            $(LIBAGG_CFLAGS)

LDADD=$(LIBOSMSCOUT_LIBS) \
      $(LIBOSMSCOUTMAP_LIBS) \
      $(LIBCAIRO_LIBS) \
      $(LIBQTGUI_LIBS) \
      $(LIBAGG_LIBS)

bin_PROGRAMS = AddressLookup \
               ResourceConsumption \
               Routing

if HAVE_LIB_CAIRO
bin_PROGRAMS += DrawMapCairo
endif

if HAVE_LIB_QTGUI
bin_PROGRAMS += DrawMapQt \
                ResourceConsumptionQt
endif

if HAVE_LIB_AGG
bin_PROGRAMS += DrawMapAgg \
                Tiler
endif

AddressLookup_SOURCES = AddressLookup.cpp

DrawMapCairo_SOURCES = DrawMapCairo.cpp

DrawMapQt_SOURCES = DrawMapQt.cpp

DrawMapAgg_SOURCES = DrawMapAgg.cpp

ResourceConsumption_SOURCES = ResourceConsumption.cpp

ResourceConsumptionQt_SOURCES = ResourceConsumptionQt.cpp

Routing_SOURCES = Routing.cpp

Tiler_SOURCES = Tiler.cpp


