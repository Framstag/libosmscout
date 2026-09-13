#ifndef LIBOSMSCOUT_MAP_SVG_FEATURES
#define LIBOSMSCOUT_MAP_SVG_FEATURES

#ifndef OSMSCOUT_MAP_SVG_HAVE_LIB_PANGO
/* libpango found */
#cmakedefine OSMSCOUT_MAP_SVG_HAVE_LIB_PANGO
#endif

#ifndef OSMSCOUT_MAP_SVG_HAVE_LIB_FONTCONFIG
/* fontconfig found, used to resolve font family names without pango */
#cmakedefine OSMSCOUT_MAP_SVG_HAVE_LIB_FONTCONFIG
#endif

#endif
