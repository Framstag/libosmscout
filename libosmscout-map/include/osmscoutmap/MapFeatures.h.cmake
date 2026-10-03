#ifndef LIBOSMSCOUT_MAP_FEATURES
#define LIBOSMSCOUT_MAP_FEATURES

#ifndef OSMSCOUT_MAP_HAVE_LIB_FREETYPE
/* The family name of a configured font file can be read */
#cmakedefine OSMSCOUT_MAP_HAVE_LIB_FREETYPE
#endif

#ifndef OSMSCOUT_DEBUG_LABEL_LAYOUTER
/* Extra debugging of label layouter */
#cmakedefine OSMSCOUT_DEBUG_LABEL_LAYOUTER
#endif

#ifndef OSMSCOUT_DEBUG_GROUNDTILES
/* Extra debugging of ground tiles rendering */
#cmakedefine OSMSCOUT_DEBUG_GROUNDTILES
#endif

#endif
