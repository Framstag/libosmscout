AM_CPPFLAGS = $(LIB_CXXFLAGS) -DOSMSCOUTMAPAGGDLL -I$(top_srcdir)/include \
              $(LIBOSMSCOUT_CFLAGS) \
              $(LIBOSMSCOUTMAP_CFLAGS) \
              $(LIBFREETYPE_CFLAGS) \
              $(LIBAGG_CFLAGS)

lib_LTLIBRARIES = libosmscoutmapagg.la
libosmscoutmapagg_la_LDFLAGS  = $(LIBOSMSCOUT_LIBS) \
                                $(LIBOSMSCOUTMAP_LIBS) \
                                $(LIBFREETYPE_LIBS) \
                                $(LIBAGG_LIBS)

libosmscoutmapagg_la_SOURCES = osmscout/MapPainterAgg.cpp

