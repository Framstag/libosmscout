#ifndef LIBOSMSCOUT_COREFEATURES_H
#define LIBOSMSCOUT_COREFEATURES_H

#ifndef OSMSCOUT_HAVE_STD_WSTRING
/* std::wstring is available */
#cmakedefine OSMSCOUT_HAVE_STD_WSTRING
#endif

#ifndef OSMSCOUT_HAVE_SSE2
/* SSE2 processor extension available */
#cmakedefine OSMSCOUT_HAVE_SSE2
#endif

#ifndef OSMSCOUT_HAVE_LIB_MARISA
/* libmarisa is available */
#cmakedefine OSMSCOUT_HAVE_LIB_MARISA
#endif

#endif
