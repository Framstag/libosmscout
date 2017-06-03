#ifndef OSMSCOUT_IMPORT_CONFIG_H
#define OSMSCOUT_IMPORT_CONFIG_H

/* Support Altivec instructions */
#cmakedefine HAVE_ALTIVEC

/* Support AVX (Advanced Vector Extensions) instructions */
#cmakedefine #undef HAVE_AVX

/* Define to 1 if you have the <dlfcn.h> header file. */
#cmakedefine HAVE_DLFCN_H 1

/* Define to 1 if you have the <fcntl.h> header file. */
#cmakedefine HAVE_FCNTL_H 1

/* Define to 1 if fseeko (and presumably ftello) exists and is declared. */
#cmakedefine HAVE_FSEEKO

/* Define to 1 if the system has the type `int16_t'. */
#cmakedefine HAVE_INT16_T 1

/* Define to 1 if the system has the type `int32_t'. */
#cmakedefine HAVE_INT32_T 1

/* Define to 1 if the system has the type `int64_t'. */
#cmakedefine HAVE_INT64_T 1

/* Define to 1 if the system has the type `int8_t'. */
#cmakedefine HAVE_INT8_T 1

/* Define to 1 if you have the <inttypes.h> header file. */
#cmakedefine HAVE_INTTYPES_H 1

/* Define to 1 if the system has the type `long long'. */
#cmakedefine HAVE_LONG_LONG 1

/* Define to 1 if you have the <memory.h> header file. */
#cmakedefine HAVE_MEMORY_H 1

/* Define to 1 if you have the `mmap' function. */
#cmakedefine HAVE_MMAP 1

/* Support mmx instructions */
#cmakedefine HAVE_MMX

/* Define to 1 if you have the `posix_fadvise' function. */
#cmakedefine HAVE_POSIX_FADVISE 1

/* Define to 1 if you have the `posix_madvise' function. */
#cmakedefine HAVE_POSIX_MADVISE 1

/* Support SSE (Streaming SIMD Extensions) instructions */
#cmakedefine HAVE_SSE

/* Support SSE2 (Streaming SIMD Extensions 2) instructions */
#cmakedefine HAVE_SSE2

/* Support SSE3 (Streaming SIMD Extensions 3) instructions */
#cmakedefine HAVE_SSE3

/* Support SSSE4.1 (Streaming SIMD Extensions 4.1) instructions */
#cmakedefine HAVE_SSE4_1

/* Support SSSE4.2 (Streaming SIMD Extensions 4.2) instructions */
#cmakedefine HAVE_SSE4_2

/* Support SSSE3 (Supplemental Streaming SIMD Extensions 3) instructions */
#cmakedefine HAVE_SSSE3

/* Define to 1 if you have the <stdint.h> header file. */
#cmakedefine HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#cmakedefine HAVE_STDLIB_H 1

/* Define to 1 if the system has the type `std::wstring'. */
#cmakedefine HAVE_STD__WSTRING 1

/* Define to 1 if you have the <strings.h> header file. */
#cmakedefine HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#cmakedefine HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#cmakedefine HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#cmakedefine HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#cmakedefine HAVE_SYS_TYPES_H 1

/* Define to 1 if the system has the type `uint16_t'. */
#cmakedefine HAVE_UINT16_T 1

/* Define to 1 if the system has the type `uint32_t'. */
#cmakedefine HAVE_UINT32_T 1

/* Define to 1 if the system has the type `uint64_t'. */
#cmakedefine HAVE_UINT64_T 1

/* Define to 1 if the system has the type `uint8_t'. */
#cmakedefine HAVE_UINT8_T 1

/* Define to 1 if you have the <unistd.h> header file. */
#cmakedefine HAVE_UNISTD_H 1

/* Define to 1 if the system has the type `unsigned long long'. */
#cmakedefine HAVE_UNSIGNED_LONG_LONG 1

/* Define to 1 or 0, depending whether the compiler supports simple visibility
   declarations. */
#cmakedefine HAVE_VISIBILITY 1

/* int16_t is available */
#cmakedefine OSMSCOUT_HAVE_INT16_T 1

/* int32_t is available */
#cmakedefine OSMSCOUT_HAVE_INT32_T 1

/* int64_t is available */
#cmakedefine OSMSCOUT_HAVE_INT64_T 1

/* int8_t is available */
#cmakedefine OSMSCOUT_HAVE_INT8_T 1

/* libmarisa detected */
#cmakedefine OSMSCOUT_HAVE_LIB_MARISA

/* long long is available */
#cmakedefine OSMSCOUT_HAVE_LONG_LONG 1

/* SSE2 processor extension available */
#cmakedefine OSMSCOUT_HAVE_SSE2 1

/* system header <stdint.h> is available */
#cmakedefine OSMSCOUT_HAVE_STDINT_H 1

/* std::wstring is available */
#cmakedefine OSMSCOUT_HAVE_STD_WSTRING 1

/* uint16_t is available */
#cmakedefine OSMSCOUT_HAVE_UINT16_T 1

/* uint32_t is available */
#cmakedefine OSMSCOUT_HAVE_UINT32_T 1

/* uint64_t is available */
#cmakedefine OSMSCOUT_HAVE_UINT64_T 1

/* uint8_t is available */
#cmakedefine OSMSCOUT_HAVE_UINT8_T 1

/* unsigned long long is available */
#cmakedefine OSMSCOUT_HAVE_ULONG_LONG 1

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
#define SIZEOF_WCHAR_T @SIZEOF_WCHAR_T@

/* Define to 1 if you have the ANSI C header files. */
#cmakedefine STDC_HEADERS 1

#endif
