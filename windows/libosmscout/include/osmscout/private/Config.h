#ifndef LIBOSMSCOUT_PRIVATE_CONFIG_H
#define LIBOSMSCOUT_PRIVATE CONFIG_H

/* Support Altivec instructions */
/* #undef HAVE_ALTIVEC */

/* Support AVX (Advanced Vector Extensions) instructions */
/* #undef HAVE_AVX */

/* Define to 1 if you have the <dlfcn.h> header file. */
/* #undef HAVE_DLFCN_H 1 */

/* Define to 1 if you have the <fcntl.h> header file. */
/* #undef HAVE_FCNTL_H 1 */

/* Define to 1 if fseeko (and presumably ftello) exists and is declared. */
/* #undef HAVE_FSEEKO */

/* Define to 1 if the system has the type `int16_t'. */
#define HAVE_INT16_T 1

/* Define to 1 if the system has the type `int32_t'. */
#define HAVE_INT32_T 1

/* Define to 1 if the system has the type `int64_t'. */
#define HAVE_INT64_T 1

/* Define to 1 if the system has the type `int8_t'. */
#define HAVE_INT8_T 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if the system has the type `long long'. */
#define HAVE_LONG_LONG 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `mmap' function. */
/* #undef HAVE_MMAP 1 */

/* Support mmx instructions */
/* #undef HAVE_MMX */

/* Define to 1 if you have the `posix_fadvise' function. */
/* #undef HAVE_POSIX_FADVISE 1 */

/* Define to 1 if you have the `posix_madvise' function. */
/* #undef HAVE_POSIX_MADVISE 1 */

/* Support SSE (Streaming SIMD Extensions) instructions */
/* #undefi HAVE_SSE */

/* Support SSE2 (Streaming SIMD Extensions 2) instructions */
/* #undef HAVE_SSE2 */

/* Support SSE3 (Streaming SIMD Extensions 3) instructions */
/* #undef HAVE_SSE3 */

/* Support SSSE4.1 (Streaming SIMD Extensions 4.1) instructions */
/* #undef HAVE_SSE4_1 */

/* Support SSSE4.2 (Streaming SIMD Extensions 4.2) instructions */
/* #undef HAVE_SSE4_2 /*/

/* Support SSSE3 (Supplemental Streaming SIMD Extensions 3) instructions */
/* #undef HAVE_SSSE3 */

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if the system has the type `std::wstring'. */
#define HAVE_STD__WSTRING 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
/* #undef HAVE_SYS_STAT_H */

/* Define to 1 if you have the <sys/time.h> header file. */
/* #undef HAVE_SYS_TIME_H */

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <thread> header file. */
#define HAVE_THREAD 1

/* Define to 1 if the system has the type `uint16_t'. */
#define HAVE_UINT16_T 1

/* Define to 1 if the system has the type `uint32_t'. */
#define HAVE_UINT32_T 1

/* Define to 1 if the system has the type `uint64_t'. */
#define HAVE_UINT64_T 1

/* Define to 1 if the system has the type `uint8_t'. */
#define HAVE_UINT8_T 1

/* Define to 1 if you have the <unistd.h> header file. */
/* #undef HAVE_UNISTD_H */

/* Define to 1 if the system has the type `unsigned long long'. */
#define HAVE_UNSIGNED_LONG_LONG 1

/* Define to 1 or 0, depending whether the compiler supports simple visibility
   declarations. */
#define HAVE_VISIBILITY 1

/* libosmscout uses special gcc compiler features to export symbols */
/* #undef OSMSCOUT_EXPORT_SYMBOLS */

/* math function atanh(double) is available */
#define OSMSCOUT_HAVE_ATANH 1

/* standard library has support for atomic */
#define OSMSCOUT_HAVE_ATOMIC 1

/* int16_t is available */
#define OSMSCOUT_HAVE_INT16_T 1

/* int32_t is available */
#define OSMSCOUT_HAVE_INT32_T 1

/* int64_t is available */
#define OSMSCOUT_HAVE_INT64_T 1

/* int8_t is available */
#define OSMSCOUT_HAVE_INT8_T 1

/* libmarisa detected */
/* #undef OSMSCOUT_HAVE_LIB_MARISA */

/* math function log2(double) is available */
#define OSMSCOUT_HAVE_LOG2 1

/* long long is available */
#define OSMSCOUT_HAVE_LONG_LONG 1

/* math function lround(double) is available */
#define OSMSCOUT_HAVE_LROUND 1

/* standard library has support for mutex */
#define OSMSCOUT_HAVE_MUTEX 1

/* SSE2 processor extension available */
#define OSMSCOUT_HAVE_SSE2 1

/* system header <stdint.h> is available */
#define OSMSCOUT_HAVE_STDINT_H 1

/* std::wstring is available */
#define OSMSCOUT_HAVE_STD_WSTRING 1

/* system header <thread> is available */
#define OSMSCOUT_HAVE_THREAD 1

/* uint16_t is available */
#define OSMSCOUT_HAVE_UINT16_T 1

/* uint32_t is available */
#define OSMSCOUT_HAVE_UINT32_T 1

/* uint64_t is available */
#define OSMSCOUT_HAVE_UINT64_T 1

/* uint8_t is available */
#define OSMSCOUT_HAVE_UINT8_T 1

/* unsigned long long is available */
#define OSMSCOUT_HAVE_ULONG_LONG 1

/* libosmscout needs to include <assert.h> */
/* #undef OSMSCOUT_REQUIRES_ASSERTH */

/* libosmscout needs to include <math.h> */
#define OSMSCOUT_REQUIRES_MATHH 1

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

#endif