AM_CPPFLAGS = $(LIB_CXXFLAGS) \
              -DOSMSCOUTMAPAGGDLL -I$(top_srcdir)/include \
              $(LIBOSMSCOUTMAP_CFLAGS) \
              $(LIBOSMSCOUT_CFLAGS) \
              $(LIBAGG_CFLAGS) \
              $(LIBFREETYPE_CFLAGS)

lib_LTLIBRARIES = libosmscoutmapagg.la
libosmscoutmapagg_la_LDFLAGS  = $(LIBOSMSCOUTMAP_LIBS) \
                                $(LIBOSMSCOUT_LIBS) \
                                $(LIBAGG_LIBS) \
                                $(LIBFREETYPE_LIBS)

libosmscoutmapagg_la_SOURCES = osmscout/MapPainterAgg.cpp

