AM_CPPFLAGS = $(LIB_CXXFLAGS) \
               -DOSMSCOUTMAPDLL -I$(top_srcdir)/include \
              $(LIBOSMSCOUT_CFLAGS)

lib_LTLIBRARIES = libosmscoutmap.la
libosmscoutmap_la_LDFLAGS  = -no-undefined $(LIBOSMSCOUT_LIBS)

libosmscoutmap_la_SOURCES = osmscout/oss/Scanner.cpp \
                            osmscout/oss/Parser.cpp \
                            osmscout/MapPainter.cpp \
                            osmscout/MapParameter.cpp \
                            osmscout/StyleConfig.cpp \
                            osmscout/MapService.cpp
