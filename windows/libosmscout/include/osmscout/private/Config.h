#ifndef LIBOSMSCOUT_CONFIG_H
#define LIBOSMSCOUT_CONFIG_H
/* include/osmscout/private/Config.h.  Generated from Config.h.in by configure.  */
/* include/osmscout/private/Config.h.in.  Generated from configure.ac by autoheader.  */

/* Define to 1 if you have the declaration of `atanh(double)', and to 0 if you
   don't. */
#undef HAVE_DECL_ATANH

/* Define to 1 if you have the declaration of `log2(double)', and to 0 if you
   don't. */
#undef HAVE_DECL_LOG2

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `mmap' function. */
#undef HAVE_MMAP

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#undef HAVE_SYS_TIME_H

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

#if defined(_MSC_VER) && (_MSC_VER < 1600)
/* system header <stdint.h> is not available */
#undef OSMSCOUT_HAVE_STDINT_H

/* int8_t is available */
#undef HAVE_INT8_T
#undef OSMSCOUT_HAVE_INT8_T

/* uint8_t is available */
#undef HAVE_UINT8_T
#undef OSMSCOUT_HAVE_UINT8_T

/* int16_t is available */
#undef OSMSCOUT_HAVE_INT16_T

/* uint16_t is available */
#undef HAVE_UINT16_T
#undef OSMSCOUT_HAVE_UINT16_T

/* int32_t is available */
#undef HAVE_INT32_T
#undef OSMSCOUT_HAVE_INT32_T

/* uint32_t is available */
#undef HAVE_UINT32_T
#undef OSMSCOUT_HAVE_UINT32_T

/* int64_t is available */
#undef HAVE_INT64_T
#undef OSMSCOUT_HAVE_INT64_T

/* uint64_t is available */
#undef HAVE_UINT64_T
#undef OSMSCOUT_HAVE_UINT64_T

#else
/* system header <stdint.h> is available */
#define OSMSCOUT_HAVE_STDINT_H 1

/* int8_t is available */
#define HAVE_INT8_T 1
#define OSMSCOUT_HAVE_INT8_T 1

/* uint8_t is available */
#define HAVE_UINT8_T 1
#define OSMSCOUT_HAVE_UINT8_T 1

/* int16_t is available */
#define HAVE_INT16_T 1
#define OSMSCOUT_HAVE_INT16_T 1

/* uint16_t is available */
#define HAVE_UINT16_T 1
#define OSMSCOUT_HAVE_UINT16_T 1

/* int32_t is available */
#define HAVE_INT32_T 1
#define OSMSCOUT_HAVE_INT32_T 1

/* uint32_t is available */
#define HAVE_UINT32_T 1
#define OSMSCOUT_HAVE_UINT32_T 1

/* int64_t is available */
#define HAVE_INT64_T 1
#define OSMSCOUT_HAVE_INT64_T 1

/* uint64_t is available */
#define HAVE_UINT64_T 1
#define OSMSCOUT_HAVE_UINT64_T 1

#endif

/* Define to 1 if you have the <unistd.h> header file. */
#undef HAVE_UNISTD_H

/* Define to 1 if you have the <unordered_map> header file. */
#define HAVE_UNORDERED_MAP 0

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* libosmscout uses special gcc compiler features to export symbols */
#define OSMSCOUT_EXPORT_SYMBOLS 1

/* system header <stdint.h> is available */
#undef OSMSCOUT_HAVE_STDINT_H

/* system header <unordered_map> is available */
#undef OSMSCOUT_HAVE_UNORDERED_MAP

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "tim@teulings.org"

/* Define to the full name of this package. */
#define PACKAGE_NAME "libosmscout"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "libosmscout 0.1"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "libosmscout"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "0.1"

/* The size of `wchar_t', as computed by sizeof. */
#define SIZEOF_WCHAR_T 4

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */

/* Enable SSE Math */
/* dont use "#if _M_IX86_FP >= 2" since that can mess up templats in projects using this library which compile without this flag */
#define USE_SSE2_MATH

#endif LIBOSMSCOUT_CONFIG_H
