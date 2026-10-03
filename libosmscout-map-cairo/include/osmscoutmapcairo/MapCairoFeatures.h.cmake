#ifndef LIBOSMSCOUT_MAP_CAIRO_FEATURES
#define LIBOSMSCOUT_MAP_CAIRO_FEATURES

#ifndef OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO
/* The cairo backend can make use of pango */
#cmakedefine OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO
#endif

#ifndef OSMSCOUT_MAP_CAIRO_HAVE_LIB_FREETYPE
/* The configured font file is loaded to draw and measure its face directly */
#cmakedefine OSMSCOUT_MAP_CAIRO_HAVE_LIB_FREETYPE
#endif

#ifndef OSMSCOUT_MAP_CAIRO_HAVE_LIB_FONTCONFIG
/* A configured font file the text stack cannot be handed is registered with the font configuration */
#cmakedefine OSMSCOUT_MAP_CAIRO_HAVE_LIB_FONTCONFIG
#endif

#endif
