bin_PROGRAMS = AddressLookup \
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
                ResourceConsumptionQt
endif

if HAVE_LIB_OSMSCOUTMAPAGG
bin_PROGRAMS += DrawMapAgg \
                Tiler
endif

if HAVE_LIB_OSMSCOUTMAPOPENGL
if HAVE_LIB_GLUT
bin_PROGRAMS += DrawMapOpenGL
endif
endif

AddressLookup_SOURCES = AddressLookup.cpp
AddressLookup_CXXFLAGS = $(LIBOSMSCOUT_CFLAGS)
AddressLookup_LDADD = $(LIBOSMSCOUT_LIBS)

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

DrawMapOpenGL_SOURCES = DrawMapOpenGL.cpp
DrawMapOpenGL_CXXFLAGS = $(LIBOSMSCOUTMAPOPENGL_CFLAGS) \
                         $(LIBOSMSCOUTMAP_CFLAGS) \
                         $(LIBOSMSCOUT_CFLAGS) \
                         $(GLUT_CFLAGS)
DrawMapOpenGL_LDADD = $(LIBOSMSCOUTMAPOPENGL_LIBS) \
                      $(LIBOSMSCOUTMAP_LIBS) \
                      $(LIBOSMSCOUT_LIBS) \
                      $(GLUT_LIBS)


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
PerformanceTest_CXXFLAGS = $(LIBOSMSCOUTMAPCAIRO_CFLAGS) \
                           $(LIBOSMSCOUTMAP_CFLAGS) \
                           $(LIBOSMSCOUT_CFLAGS)
PerformanceTest_LDADD = $(LIBOSMSCOUTMAPCAIRO_LIBS) \
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

