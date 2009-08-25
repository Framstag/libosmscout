AM_CPPFLAGS = -I$(top_srcdir)/include
AM_LDFLAGS  = ../src/libosmscout.la

check_PROGRAMS = EncodeNumber \
                 NumberSet

TESTS = $(check_PROGRAMS)

EncodeNumber_SOURCES = EncodeNumber.cpp
EncodeNumber_DEPENDENCIES = $(top_srcdir)/src/libosmscout.la

NumberSet_SOURCES = NumberSet.cpp
NumberSet_DEPENDENCIES = $(top_srcdir)/src/libosmscout.la

