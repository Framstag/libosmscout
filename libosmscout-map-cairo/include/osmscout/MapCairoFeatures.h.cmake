#ifndef LIBOSMSCOUT_MAP_CAIRO_FEATURES
#define LIBOSMSCOUT_MAP_CAIRO_FEATURES

#ifndef OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO
/* The cairo backend can make use of pango */
#cmakedefine OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO
#endif

#endif
