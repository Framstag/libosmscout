AM_CPPFLAGS = $(LIB_CXXFLAGS) \
              -DOSMSCOUTMAPAGGDLL -I$(top_srcdir)/include \
              $(LIBOSMSCOUT_CFLAGS) \
              $(LIBAGG_CFLAGS) \
              $(LIBFREETYPE_CFLAGS)

lib_LTLIBRARIES = libosmscoutmapagg.la
libosmscoutmapagg_la_LDFLAGS  = -no-undefined \
                                $(LIBOSMSCOUT_LIBS) \
                                $(LIBAGG_LIBS) \
                                $(LIBFREETYPE_LIBS)

libosmscoutmapagg_la_SOURCES = osmscout/MapPainterAgg.cpp

