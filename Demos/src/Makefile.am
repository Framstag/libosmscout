AM_CXXFLAGS=$(LIBOSMSCOUT_CFLAGS) \
            $(LIBOSMSCOUTMAP_CFLAGS) \
            $(LIBOSMSCOUTMAPSVG_CFLAGS) \
            $(LIBOSMSCOUTMAPQT_CFLAGS) \
            $(LIBCAIRO_CFLAGS) \
            $(LIBAGG_CFLAGS)

LDADD=$(LIBOSMSCOUT_LIBS) \
      $(LIBOSMSCOUTMAP_LIBS) \
      $(LIBOSMSCOUTMAPSVG_LIBS) \
      $(LIBOSMSCOUTMAPQT_LIBS) \
      $(LIBCAIRO_LIBS) \
      $(LIBAGG_LIBS)

bin_PROGRAMS = AddressLookup \
               ResourceConsumption \
               Routing \
               LookupPOI

if HAVE_LIB_OSMSCOUTMAPSVG
bin_PROGRAMS += DrawMapSVG
endif

if HAVE_LIB_CAIRO
bin_PROGRAMS += DrawMapCairo
endif

if HAVE_LIB_OSMSCOUTMAPQT
bin_PROGRAMS += DrawMapQt \
                ResourceConsumptionQt
endif

if HAVE_LIB_AGG
bin_PROGRAMS += DrawMapAgg \
                Tiler
endif

AddressLookup_SOURCES = AddressLookup.cpp

DrawMapAgg_SOURCES = DrawMapAgg.cpp

DrawMapCairo_SOURCES = DrawMapCairo.cpp

DrawMapQt_SOURCES = DrawMapQt.cpp

DrawMapSVG_SOURCES = DrawMapSVG.cpp

LookupPOI_SOURCES = LookupPOI.cpp

ResourceConsumption_SOURCES = ResourceConsumption.cpp

ResourceConsumptionQt_SOURCES = ResourceConsumptionQt.cpp

Routing_SOURCES = Routing.cpp

Tiler_SOURCES = Tiler.cpp


