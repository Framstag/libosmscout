AM_CPPFLAGS = $(LIB_CXXFLAGS) \
              -DOSMSCOUTMAPOPENGLDLL -I$(top_srcdir)/include \
              $(LIBOSMSCOUT_CFLAGS) \
              $(GLUT_CFLAGS) \
              $(LIBOPENGL_CFLAGS) \
              $(LIBFREETYPE_CFLAGS)

lib_LTLIBRARIES = libosmscoutmapopengl.la
libosmscoutmapopengl_la_LDFLAGS  = -no-undefined \
                                  $(LIBOSMSCOUT_LIBS) \
                                  $(GLUT_LIBS) \
                                  $(LIBOPENGL_LIBS) \
                                  $(LIBFREETYPE_LIBS)

libosmscoutmapopengl_la_SOURCES = osmscout/MapPainterOpenGL.cpp

