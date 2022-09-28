#ifndef LIBOSMSCOUT_IMPORT_FEATURES
#define LIBOSMSCOUT_IMPORT_FEATURES

#ifndef OSMSCOUT_IMPORT_HAVE_LIB_MARISA
/* libmarisa is available */
#cmakedefine OSMSCOUT_IMPORT_HAVE_LIB_MARISA
#endif

#ifndef OSMSCOUT_DEBUG_COASTLINE
/* Extra debugging of coastline evaluation */
#cmakedefine OSMSCOUT_DEBUG_COASTLINE
#endif

#ifndef OSMSCOUT_DEBUG_TILING
/* Extra debugging of water index tiling */
#cmakedefine OSMSCOUT_DEBUG_TILING
#endif

#endif
