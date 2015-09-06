AM_CPPFLAGS = -I$(top_srcdir)/include
AM_LDFLAGS  = ../src/libosmscout.la

check_PROGRAMS = AccessParse \
                 BitsAndBytesNeeded \
                 EncodeNumber \
                 FileScannerWriter \
                 GeoCoordParse \
                 NumberSet \
                 ScanConversion

TESTS = $(check_PROGRAMS)

AccessParse_SOURCES = AccessParse.cpp
AccessParse_DEPENDENCIES = $(top_srcdir)/src/libosmscout.la

BitsAndBytesNeeded_SOURCES = BitsAndBytesNeeded.cpp
BitsAndBytesNeeded_DEPENDENCIES = $(top_srcdir)/src/libosmscout.la

EncodeNumber_SOURCES = EncodeNumber.cpp
EncodeNumber_DEPENDENCIES = $(top_srcdir)/src/libosmscout.la

FileScannerWriter_SOURCES = FileScannerWriter.cpp
FileScannerWriter_DEPENDENCIES = $(top_srcdir)/src/libosmscout.la

GeoCoordParse_SOURCES = GeoCoordParse.cpp
GeoCoordParse_DEPENDENCIES = $(top_srcdir)/src/libosmscout.la

NumberSet_SOURCES = NumberSet.cpp
NumberSet_DEPENDENCIES = $(top_srcdir)/src/libosmscout.la

ScanConversion_SOURCES = ScanConversion.cpp
ScanConversion_DEPENDENCIES = $(top_srcdir)/src/libosmscout.la

