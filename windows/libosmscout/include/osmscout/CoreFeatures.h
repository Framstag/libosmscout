#ifndef LIBOSMSCOUT_COREFEATURES_H
#define LIBOSMSCOUT_COREFEATURES_H

/* libosmscout uses special gcc compiler features to export symbols */
/* #undef OSMSCOUT_EXPORT_SYMBOLS */

/* system header <stdint.h> is available */
#define OSMSCOUT_HAVE_STDINT_H 1

/* std::wstring is available */
#define OSMSCOUT_HAVE_STD_WSTRING 1

/* int8_t is available */
#define OSMSCOUT_HAVE_INT8_T 1

/* uint8_t is available */
#define OSMSCOUT_HAVE_UINT8_T 1

/* int16_t is available */
#define OSMSCOUT_HAVE_INT16_T 1

/* uint16_t is available */
#define OSMSCOUT_HAVE_UINT16_T 1

/* int32_t is available */
#define OSMSCOUT_HAVE_INT32_T 1

/* uint32_t is available */
#define OSMSCOUT_HAVE_UINT32_T 1

/* int64_t is available */
#define OSMSCOUT_HAVE_INT64_T 1

/* uint64_t is available */
#define OSMSCOUT_HAVE_UINT64_T 1

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

/* system header <thread> is available */
#define OSMSCOUT_HAVE_THREAD 1

/* libmarisa is available */
/* #undef OSMSCOUT_HAVE_LIB_MARISA 1 */

/* long long is available */
#define OSMSCOUT_HAVE_LONG_LONG 1

/* unsigned long long is available */
#define OSMSCOUT_HAVE_ULONG_LONG 1

#endif