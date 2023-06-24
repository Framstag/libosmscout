#ifndef LIBOSMSCOUT_COREFEATURES_H
#define LIBOSMSCOUT_COREFEATURES_H

#ifndef OSMSCOUT_HAVE_SSE2
/* SSE2 processor extension available */
#cmakedefine OSMSCOUT_HAVE_SSE2
#endif

#ifndef OSMSCOUT_HAVE_LIB_MARISA
/* libmarisa is available */
#cmakedefine OSMSCOUT_HAVE_LIB_MARISA
#endif

#ifndef OSMSCOUT_DEBUG_ROUTING
/* Extra debugging of routing */
#cmakedefine OSMSCOUT_DEBUG_ROUTING
#endif

#endif
