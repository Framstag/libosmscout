bin_PROGRAMS = DumpOSS \
               LocationDescription \
               LocationLookup \
               ReverseLocationLookup \
               PerformanceTest \
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
                ResourceConsumptionQt \
                RoutingAnimation
endif

if HAVE_LIB_OSMSCOUTMAPAGG
bin_PROGRAMS += DrawMapAgg \
                Tiler
endif

if OSMSCOUT_HAVE_LIB_MARISA
bin_PROGRAMS += LookupText
endif

DumpOSS_SOURCES = DumpOSS.cpp
DumpOSS_CXXFLAGS = $(LIBOSMSCOUTMAP_CFLAGS) \
                   $(LIBOSMSCOUT_CFLAGS)
DumpOSS_LDADD = $(LIBOSMSCOUTMAP_LIBS) \
                $(LIBOSMSCOUT_LIBS)

LocationLookup_SOURCES = LocationLookup.cpp
LocationLookup_CXXFLAGS = $(LIBOSMSCOUT_CFLAGS)
LocationLookup_LDADD = $(LIBOSMSCOUT_LIBS)

LocationDescription_SOURCES = LocationDescription.cpp
LocationDescription_CXXFLAGS = $(LIBOSMSCOUT_CFLAGS)
LocationDescription_LDADD = $(LIBOSMSCOUT_LIBS)

ReverseLocationLookup_SOURCES = ReverseLocationLookup.cpp
ReverseLocationLookup_CXXFLAGS = $(LIBOSMSCOUT_CFLAGS)
ReverseLocationLookup_LDADD = $(LIBOSMSCOUT_LIBS)

DrawMapAgg_SOURCES = DrawMapAgg.cpp
DrawMapAgg_CXXFLAGS = $(LIBOSMSCOUTMAPAGG_CFLAGS) \
                      $(LIBOSMSCOUTMAP_CFLAGS) \
                      $(LIBOSMSCOUT_CFLAGS)
DrawMapAgg_LDADD = $(LIBOSMSCOUTMAPAGG_LIBS) \
                   $(LIBOSMSCOUTMAP_LIBS) \
                   $(LIBOSMSCOUT_LIBS)

DrawMapCairo_SOURCES = DrawMapCairo.cpp
DrawMapCairo_CXXFLAGS = $(LIBOSMSCOUTMAPCAIRO_CFLAGS) \
                        $(LIBOSMSCOUTMAP_CFLAGS) \
                        $(LIBOSMSCOUT_CFLAGS)
DrawMapCairo_LDADD = $(LIBOSMSCOUTMAPCAIRO_LIBS) \
                     $(LIBOSMSCOUTMAP_LIBS) \
                     $(LIBOSMSCOUT_LIBS)

DrawMapQt_SOURCES = DrawMapQt.cpp
DrawMapQt_CXXFLAGS = -fPIC \
                     $(LIBOSMSCOUTMAPQT_CFLAGS) \
                     $(LIBOSMSCOUTMAP_CFLAGS) \
                     $(LIBOSMSCOUT_CFLAGS)
DrawMapQt_LDADD = $(LIBOSMSCOUTMAPQT_LIBS) \
                  $(LIBOSMSCOUTMAP_LIBS) \
                  $(LIBOSMSCOUT_LIBS)

DrawMapSVG_SOURCES = DrawMapSVG.cpp
DrawMapSVG_CXXFLAGS = $(LIBOSMSCOUTMAPSVG_CFLAGS) \
                      $(LIBOSMSCOUTMAP_CFLAGS) \
                      $(LIBOSMSCOUT_CFLAGS)
DrawMapSVG_LDADD = $(LIBOSMSCOUTMAPSVG_LIBS) \
                   $(LIBOSMSCOUTMAP_LIBS) \
                   $(LIBOSMSCOUT_LIBS)

LookupPOI_SOURCES = LookupPOI.cpp
LookupPOI_CXXFLAGS = $(LIBOSMSCOUT_CFLAGS)
LookupPOI_LDADD = $(LIBOSMSCOUT_LIBS)

PerformanceTest_SOURCES = PerformanceTest.cpp
PerformanceTest_CXXFLAGS = -fPIC \
                           $(LIBOSMSCOUTMAPCAIRO_CFLAGS) \
                           $(LIBOSMSCOUTMAPQT_CFLAGS) \
                           $(LIBOSMSCOUTMAP_CFLAGS) \
                           $(LIBOSMSCOUT_CFLAGS)
PerformanceTest_LDADD = $(LIBOSMSCOUTMAPCAIRO_LIBS) \
                        $(LIBOSMSCOUTMAPQT_LIBS) \
                        $(LIBOSMSCOUTMAP_LIBS) \
                        $(LIBOSMSCOUT_LIBS)

ResourceConsumption_SOURCES = ResourceConsumption.cpp
ResourceConsumption_CXXFLAGS = $(LIBOSMSCOUTMAP_CFLAGS) \
                               $(LIBOSMSCOUT_CFLAGS)
ResourceConsumption_LDADD = $(LIBOSMSCOUTMAP_LIBS) \
                            $(LIBOSMSCOUT_LIBS)

ResourceConsumptionQt_SOURCES = ResourceConsumptionQt.cpp
ResourceConsumptionQt_CXXFLAGS = -fPIC \
                                 $(LIBOSMSCOUTMAPQT_CFLAGS) \
                                 $(LIBOSMSCOUTMAP_CFLAGS) \
                                 $(LIBOSMSCOUT_CFLAGS)
ResourceConsumptionQt_LDADD = $(LIBOSMSCOUTMAPQT_LIBS) \
                              $(LIBOSMSCOUTMAP_LIBS) \
                              $(LIBOSMSCOUT_LIBS)

RoutingAnimation_SOURCES = RoutingAnimation.cpp
RoutingAnimation_CXXFLAGS = -fPIC \
                            $(LIBOSMSCOUTMAPQT_CFLAGS) \
                            $(LIBOSMSCOUTMAP_CFLAGS) \
                            $(LIBOSMSCOUT_CFLAGS)
RoutingAnimation_LDADD = $(LIBOSMSCOUTMAPQT_LIBS) \
                         $(LIBOSMSCOUTMAP_LIBS) \
                         $(LIBOSMSCOUT_LIBS)

Routing_SOURCES = Routing.cpp
Routing_CXXFLAGS = $(LIBOSMSCOUT_CFLAGS)
Routing_LDADD = $(LIBOSMSCOUT_LIBS)

Tiler_SOURCES = Tiler.cpp
Tiler_CXXFLAGS = $(LIBOSMSCOUTMAPAGG_CFLAGS) \
                 $(LIBOSMSCOUTMAP_CFLAGS) \
                 $(LIBOSMSCOUT_CFLAGS)
Tiler_LDADD = $(LIBOSMSCOUTMAPAGG_LIBS) \
              $(LIBOSMSCOUTMAP_LIBS) \
              $(LIBOSMSCOUT_LIBS)

Srtm_SOURCES = Srtm.cpp
Srtm_CXXFLAGS = $(LIBOSMSCOUT_CFLAGS)
Srtm_LDADD = $(LIBOSMSCOUT_LIBS)

LookupText_SOURCES = LookupText.cpp
LookupText_CXXFLAGS = $(LIBOSMSCOUT_CFLAGS) $(MARISA_CFLAGS)
LookupText_LDADD = $(LIBOSMSCOUT_LIBS) $(MARISA_LIBS)
