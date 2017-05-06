# interactive tests, we exclude it from CI build, check target
# WStringStringConversion works only with some locales, exclude it too
bin_PROGRAMS = CachePerformance \
               CoordinateEncoding \
               ReaderScannerPerformance \
               MultiDBRouting  \
               ThreadedDatabase \
               WStringStringConversion

check_PROGRAMS = CalculateResolution \
		 ColorParse \
                 NumberSetPerformance \
                 WorkQueue \
                 MapRotate \
                 Geometry \
                 AccessParse \
                 BitsAndBytesNeeded \
                 EncodeNumber \
                 FileScannerWriter \
                 GeoCoordParse \
                 NumberSet \
                 ScanConversion \
                 TransPolygon \
		 GeoBox

AM_CPPFLAGS = $(LIB_CXXFLAGS) \
              -I$(top_srcdir)/include

AccessParse_SOURCES = AccessParse.cpp
AccessParse_CXXFLAGS = $(LIBOSMSCOUT_CFLAGS)
AccessParse_LDADD = $(LIBOSMSCOUT_LIBS)

BitsAndBytesNeeded_SOURCES = BitsAndBytesNeeded.cpp
BitsAndBytesNeeded_CXXFLAGS = $(LIBOSMSCOUT_CFLAGS)
BitsAndBytesNeeded_LDADD = $(LIBOSMSCOUT_LIBS)

ColorParse_SOURCES = ColorParse.cpp
ColorParse_CXXFLAGS = $(LIBOSMSCOUT_CFLAGS)
ColorParse_LDADD = $(LIBOSMSCOUT_LIBS)

EncodeNumber_SOURCES = EncodeNumber.cpp
EncodeNumber_CXXFLAGS = $(LIBOSMSCOUT_CFLAGS)
EncodeNumber_LDADD = $(LIBOSMSCOUT_LIBS)

FileScannerWriter_SOURCES = FileScannerWriter.cpp
FileScannerWriter_CXXFLAGS = $(LIBOSMSCOUT_CFLAGS)
FileScannerWriter_LDADD = $(LIBOSMSCOUT_LIBS)

GeoCoordParse_SOURCES = GeoCoordParse.cpp
GeoCoordParse_CXXFLAGS = $(LIBOSMSCOUT_CFLAGS)
GeoCoordParse_LDADD = $(LIBOSMSCOUT_LIBS)

NumberSet_SOURCES = NumberSet.cpp
NumberSet_CXXFLAGS = $(LIBOSMSCOUT_CFLAGS)
NumberSet_LDADD = $(LIBOSMSCOUT_LIBS)

ScanConversion_SOURCES = ScanConversion.cpp
ScanConversion_CXXFLAGS = $(LIBOSMSCOUT_CFLAGS)
ScanConversion_LDADD = $(LIBOSMSCOUT_LIBS)

WStringStringConversion_SOURCES = WStringStringConversion.cpp
WStringStringConversion_CXXFLAGS = $(LIBOSMSCOUT_CFLAGS)
WStringStringConversion_LDADD = $(LIBOSMSCOUT_LIBS)

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

TransPolygon_SOURCES = TransPolygon.cpp
TransPolygon_CXXFLAGS = $(LIBOSMSCOUT_CFLAGS)
TransPolygon_LDADD = $(LIBOSMSCOUT_LIBS)

GeoBox_SOURCES = GeoBox.cpp
GeoBox_CXXFLAGS = $(LIBOSMSCOUT_CFLAGS)
GeoBox_LDADD = $(LIBOSMSCOUT_LIBS)

TESTS = $(check_PROGRAMS)

