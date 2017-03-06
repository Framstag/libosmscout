bin_PROGRAMS = CachePerformance \
               CalculateResolution \
               CoordinateEncoding \
               NumberSetPerformance \
               ReaderScannerPerformance \
               ThreadedDatabase \
               WorkQueue \
	       MapRotate \
               MultiDBRouting \
	       Geometry

AM_CPPFLAGS = $(LIB_CXXFLAGS) \
              -I$(top_srcdir)/include

CachePerformance_SOURCES = CachePerformance.cpp
CachePerformance_CXXFLAGS = $(LIBOSMSCOUT_CFLAGS)
CachePerformance_LDADD = $(LIBOSMSCOUT_LIBS)

CalculateResolution_SOURCES = CalculateResolution.cpp
CalculateResolution_CXXFLAGS = $(LIBOSMSCOUT_CFLAGS)
CalculateResolution_LDADD = $(LIBOSMSCOUT_LIBS)

CoordinateEncoding_SOURCES = CoordinateEncoding.cpp
CoordinateEncoding_CXXFLAGS = $(LIBOSMSCOUT_CFLAGS)
CoordinateEncoding_LDADD = $(LIBOSMSCOUT_LIBS)

NumberSetPerformance_SOURCES = NumberSetPerformance.cpp
NumberSetPerformance_CXXFLAGS = $(LIBOSMSCOUT_CFLAGS)
NumberSetPerformance_LDADD = $(LIBOSMSCOUT_LIBS)

ReaderScannerPerformance_SOURCES = ReaderScannerPerformance.cpp
ReaderScannerPerformance_CXXFLAGS = $(LIBOSMSCOUT_CFLAGS)
ReaderScannerPerformance_LDADD = $(LIBOSMSCOUT_LIBS)

ThreadedDatabase_SOURCES = ThreadedDatabase.cpp
ThreadedDatabase_CXXFLAGS = $(LIBOSMSCOUT_CFLAGS) $(LIBOSMSCOUTMAP_CFLAGS)
ThreadedDatabase_LDADD = $(LIBOSMSCOUT_LIBS) $(LIBOSMSCOUTMAP_LIBS)

WorkQueue_SOURCES = WorkQueue.cpp
WorkQueue_CXXFLAGS = $(LIBOSMSCOUT_CFLAGS) $(LIBOSMSCOUTMAP_CFLAGS)
WorkQueue_LDADD = $(LIBOSMSCOUT_LIBS) $(LIBOSMSCOUTMAP_LIBS)

MapRotate_SOURCES = MapRotate.cpp
MapRotate_CXXFLAGS = $(LIBOSMSCOUT_CFLAGS) $(LIBOSMSCOUTMAP_CFLAGS)
MapRotate_LDADD = $(LIBOSMSCOUT_LIBS) $(LIBOSMSCOUTMAP_LIBS)

MultiDBRouting_SOURCES = MultiDBRouting.cpp
MultiDBRouting_CXXFLAGS = $(LIBOSMSCOUT_CFLAGS)
MultiDBRouting_LDADD = $(LIBOSMSCOUT_LIBS)

Geometry_SOURCES = Geometry.cpp
Geometry_CXXFLAGS = $(LIBOSMSCOUT_CFLAGS)
Geometry_LDADD = $(LIBOSMSCOUT_LIBS)

if HAVE_LIB_OSMSCOUTMAPQT
bin_PROGRAMS += DrawTextQt

moc_%.cpp: $(top_srcdir)/include/%.h
	@MOC@ -o$@ $<

CLEANFILES = moc_DrawWindow.cpp
endif

DrawTextQt_SOURCES = DrawTextQt.cpp \
                     moc_DrawWindow.cpp
DrawTextQt_CXXFLAGS = -fPIC \
                      $(LIBOSMSCOUTMAPQT_CFLAGS) \
                      $(LIBOSMSCOUTMAP_CFLAGS) \
                      $(LIBOSMSCOUT_CFLAGS)
DrawTextQt_LDADD = $(LIBOSMSCOUTMAPQT_LIBS) \
                   $(LIBOSMSCOUTMAP_LIBS) \
                   $(LIBOSMSCOUT_LIBS)
