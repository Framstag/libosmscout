#ifndef LIBOSMSCOUT_COREFEATURES_H
#define LIBOSMSCOUT_COREFEATURES_H

/* libosmscout uses special gcc compiler features to export symbols */
/* #undef OSMSCOUT_EXPORT_SYMBOLS */

/* std::wstring is available */
#define OSMSCOUT_HAVE_STD_WSTRING 1

/* SSE2 processor extension available */
#define OSMSCOUT_HAVE_SSE2 1

/* libmarisa is available */
/* #undef OSMSCOUT_HAVE_LIB_MARISA 1 */

/* long long is available */
#define OSMSCOUT_HAVE_LONG_LONG 1

/* unsigned long long is available */
#define OSMSCOUT_HAVE_ULONG_LONG 1

#endif
