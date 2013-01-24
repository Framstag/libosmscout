AM_CPPFLAGS=$(LIBOSMSCOUTMAPSVG_CFLAGS) \
            $(LIBOSMSCOUTMAPCAIRO_CFLAGS) \
            $(LIBOSMSCOUTMAPQT_CFLAGS) \
            $(LIBOSMSCOUTMAPAGG_CFLAGS) \
            $(LIBOSMSCOUTMAP_CFLAGS) \
            $(LIBOSMSCOUT_CFLAGS)

bin_PROGRAMS = AddressLookup \
               DumpData \
               ResourceConsumption \
               Routing \
               LookupPOI \
               Srtm

if HAVE_LIB_OSMSCOUTMAPSVG
bin_PROGRAMS += DrawMapSVG
endif

if HAVE_LIB_OSMSCOUTMAPCAIRO
bin_PROGRAMS += DrawMapCairo
endif

if HAVE_LIB_OSMSCOUTMAPQT
bin_PROGRAMS += DrawMapQt \
                ResourceConsumptionQt
endif

if HAVE_LIB_OSMSCOUTMAPAGG
bin_PROGRAMS += DrawMapAgg \
                Tiler
endif

AddressLookup_SOURCES = AddressLookup.cpp
AddressLookup_LDADD = $(LIBOSMSCOUT_LIBS)

DrawMapAgg_SOURCES = DrawMapAgg.cpp
DrawMapAgg_LDADD = $(LIBOSMSCOUTMAPAGG_LIBS) \
                   $(LIBOSMSCOUTMAP_LIBS) \
                   $(LIBOSMSCOUT_LIBS)

DrawMapCairo_SOURCES = DrawMapCairo.cpp
DrawMapCairo_LDADD = $(LIBOSMSCOUTMAPCAIRO_LIBS) \
                     $(LIBOSMSCOUTMAP_LIBS) \
                     $(LIBOSMSCOUT_LIBS)

DrawMapQt_SOURCES = DrawMapQt.cpp
DrawMapQt_LDADD = $(LIBOSMSCOUTMAPQT_LIBS) \
                  $(LIBOSMSCOUTMAP_LIBS) \
                  $(LIBOSMSCOUT_LIBS)

DrawMapSVG_SOURCES = DrawMapSVG.cpp
DrawMapSVG_LDADD = $(LIBOSMSCOUTMAPSVG_LIBS) \
                   $(LIBOSMSCOUTMAP_LIBS) \
                   $(LIBOSMSCOUT_LIBS)

DumpData_SOURCES = DumpData.cpp
DumpData_LDADD = $(LIBOSMSCOUT_LIBS)

LookupPOI_SOURCES = LookupPOI.cpp
LookupPOI_LDADD = $(LIBOSMSCOUT_LIBS)

ResourceConsumption_SOURCES = ResourceConsumption.cpp
ResourceConsumption_LDADD = $(LIBOSMSCOUT_LIBS) \
                            $(LIBOSMSCOUTMAP_LIBS)

ResourceConsumptionQt_SOURCES = ResourceConsumptionQt.cpp
ResourceConsumptionQt_LDADD = $(LIBOSMSCOUTMAPQT_LIBS) \
                              $(LIBOSMSCOUTMAP_LIBS) \
                              $(LIBOSMSCOUT_LIBS)

Routing_SOURCES = Routing.cpp
Routing_LDADD = $(LIBOSMSCOUT_LIBS)

Tiler_SOURCES = Tiler.cpp
Tiler_LDADD = $(LIBOSMSCOUTMAPAGG_LIBS) \
              $(LIBOSMSCOUTMAP_LIBS) \
              $(LIBOSMSCOUT_LIBS)

Srtm_SOURCES = Srtm.cpp
Srtm_LDADD = $(LIBOSMSCOUT_LIBS)

