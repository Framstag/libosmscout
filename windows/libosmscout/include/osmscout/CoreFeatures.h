#ifndef LIBOSMSCOUT_COREFEATURES_H
#define LIBOSMSCOUT_COREFEATURES_H

/* libosmscout uses special gcc compiler features to export symbols */
/* #undef OSMSCOUT_EXPORT_SYMBOLS */

/* system header <stdint.h> is available */
#define OSMSCOUT_HAVE_STDINT_H 1

/* std::wstring is available */
#define OSMSCOUT_HAVE_STD_WSTRING 1

/* SSE2 processor extension available */
#define OSMSCOUT_HAVE_SSE2 1

/* libosmscout needs to include <assert.h> */
/* #undef OSMSCOUT_REQUIRES_ASSERTH */

/* libosmscout needs to include <math.h> */
#define OSMSCOUT_REQUIRES_MATHH 1

/* math function log2(double) is available */
#define OSMSCOUT_HAVE_LOG2 1

/* math function atanh(double) is available */
#define OSMSCOUT_HAVE_ATANH 1

/* math function lround(double) is available */
#define OSMSCOUT_HAVE_LROUND 1

/* libmarisa is available */
/* #undef OSMSCOUT_HAVE_LIB_MARISA 1 */

/* long long is available */
#define OSMSCOUT_HAVE_LONG_LONG 1

/* unsigned long long is available */
#define OSMSCOUT_HAVE_ULONG_LONG 1

#endif
