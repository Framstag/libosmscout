AM_CPPFLAGS = $(LIB_CXXFLAGS) \
              -DOSMSCOUTMAPOPENGLDLL -I$(top_srcdir)/include \
              $(LIBOSMSCOUTMAP_CFLAGS) \
              $(LIBOSMSCOUT_CFLAGS)

lib_LTLIBRARIES = libosmscoutmapopengl.la
libosmscoutmapopengl_la_LDFLAGS  = -no-undefined \
                                  $(LIBOSMSCOUTMAP_LIBS) \
                                  $(LIBOSMSCOUT_LIBS)

libosmscoutmapopengl_la_SOURCES = osmscout/MapPainterOpenGL.cpp

