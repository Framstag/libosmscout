AM_CPPFLAGS = $(LIB_CXXFLAGS) \
              -I$(top_srcdir)/include \
              $(LIBOSMSCOUTMAP_CFLAGS) \
              $(LIBOSMSCOUT_CFLAGS) \
              -stdlib=libc++

AM_OBJCXXFLAGS = -std=c++11 -fobjc-arc -stdlib=libc++

lib_LTLIBRARIES = libosmscoutmapiosx.la
libosmscoutmapiosx_la_LDFLAGS  = -no-undefined \
                               $(LIBOSMSCOUTMAP_LIBS) \
                               $(LIBOSMSCOUT_LIBS) \
                               -framework Foundation \
                               -framework Cocoa \
                               -framework AppKit \
                               -framework CoreData

libosmscoutmapiosx_la_SOURCES = osmscout/MapPainterIOS.mm
