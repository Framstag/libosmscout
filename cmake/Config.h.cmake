#ifndef @OSMSCOUT_PRIVATE_CONFIG_HEADER_NAME@_PRIVATE_CONFIG_H
#define @OSMSCOUT_PRIVATE_CONFIG_HEADER_NAME@_PRIVATE_CONFIG_H

/* Support Altivec instructions */
#ifndef HAVE_ALTIVEC
#cmakedefine HAVE_ALTIVEC 1
#endif

/* Support AVX (Advanced Vector Extensions) instructions */
#ifndef HAVE_AVX
#cmakedefine HAVE_AVX 1
#endif

/* Define to 1 if you have the <dlfcn.h> header file. */
#ifndef HAVE_DLFCN_H
#cmakedefine HAVE_DLFCN_H 1
#endif

/* Define to 1 if you have the <fcntl.h> header file. */
#ifndef HAVE_FCNTL_H
#cmakedefine HAVE_FCNTL_H 1
#endif

/* Define to 1 if fseeko (and presumably ftello) exists and is declared. */
#ifndef HAVE_FSEEKO
#cmakedefine HAVE_FSEEKO 1
#endif

/* Define to 1 if _fseeki64 exists and is declared. */
#ifndef HAVE__FSEEKI64
#cmakedefine HAVE__FSEEKI64 1
#endif

/* Define to 1 if _ftelli64 exists and is declared. */
#ifndef HAVE__FTELLI64
#cmakedefine HAVE__FTELLI64 1
#endif

#cmakedefine _LARGEFILE_SOURCE
#cmakedefine _LARGE_FILES
#cmakedefine _FILE_OFFSET_BITS @_FILE_OFFSET_BITS@

/* Define to 1 if the system has the type `int16_t'. */
#ifndef HAVE_INT16_T
#cmakedefine HAVE_INT16_T 1
#endif

/* Define to 1 if the system has the type `int32_t'. */
#ifndef HAVE_INT32_T
#cmakedefine HAVE_INT32_T 1
#endif

/* Define to 1 if the system has the type `int64_t'. */
#ifndef HAVE_INT64_T
#cmakedefine HAVE_INT64_T 1
#endif

/* Define to 1 if the system has the type `int8_t'. */
#ifndef HAVE_INT8_T
#cmakedefine HAVE_INT8_T 1
#endif

/* Define to 1 if you have the <inttypes.h> header file. */
#ifndef HAVE_INTTYPES_H
#cmakedefine HAVE_INTTYPES_H 1
#endif

/* Define to 1 if the system has the type `long long'. */
#ifndef HAVE_LONG_LONG
#cmakedefine HAVE_LONG_LONG 1
#endif

/* Define to 1 if you have the <memory.h> header file. */
#ifndef HAVE_MEMORY_H
#cmakedefine HAVE_MEMORY_H 1
#endif

/* Define to 1 if you have the `mmap' function. */
#ifndef HAVE_MMAP
#cmakedefine HAVE_MMAP 1
#endif

/* Support mmx instructions */
#ifndef HAVE_MMX
#cmakedefine HAVE_MMX 1
#endif

/* Define to 1 if you have the `posix_fadvise' function. */
#ifndef HAVE_POSIX_FADVISE
#cmakedefine HAVE_POSIX_FADVISE 1
#endif

/* Define to 1 if you have the `posix_madvise' function. */
#ifndef HAVE_POSIX_MADVISE
#cmakedefine HAVE_POSIX_MADVISE 1
#endif

/* Support SSE (Streaming SIMD Extensions) instructions */
#ifndef HAVE_SSE
#cmakedefine HAVE_SSE 1
#endif

/* Support SSE2 (Streaming SIMD Extensions 2) instructions */
#ifndef HAVE_SSE2
#cmakedefine HAVE_SSE2 1
#endif

/* Support SSE3 (Streaming SIMD Extensions 3) instructions */
#ifndef HAVE_SSE3
#cmakedefine HAVE_SSE3 1
#endif

/* Support SSSE4.1 (Streaming SIMD Extensions 4.1) instructions */
#ifndef HAVE_SSE4_1
#cmakedefine HAVE_SSE4_1 1
#endif

/* Support SSSE4.2 (Streaming SIMD Extensions 4.2) instructions */
#ifndef HAVE_SSE4_2
#cmakedefine HAVE_SSE4_2 1
#endif

/* Support SSSE3 (Supplemental Streaming SIMD Extensions 3) instructions */
#ifndef HAVE_SSSE3
#cmakedefine HAVE_SSSE3 1
#endif

/* Define to 1 if you have the <stdint.h> header file. */
#ifndef HAVE_STDINT_H
#cmakedefine HAVE_STDINT_H 1
#endif

/* Define to 1 if you have the <stdlib.h> header file. */
#ifndef HAVE_STDLIB_H
#cmakedefine HAVE_STDLIB_H 1
#endif

/* Define to 1 if you have the <strings.h> header file. */
#ifndef HAVE_STRINGS_H
#cmakedefine HAVE_STRINGS_H 1
#endif

/* Define to 1 if you have the <string.h> header file. */
#ifndef HAVE_STRING_H
#cmakedefine HAVE_STRING_H 1
#endif

/* Define to 1 if you have the <sys/stat.h> header file. */
#ifndef HAVE_SYS_STAT_H
#cmakedefine HAVE_SYS_STAT_H 1
#endif

/* Define to 1 if you have the <sys/time.h> header file. */
#ifndef HAVE_SYS_TIME_H
#cmakedefine HAVE_SYS_TIME_H 1
#endif

/* Define to 1 if you have the <sys/types.h> header file. */
#ifndef HAVE_SYS_TYPES_H
#cmakedefine HAVE_SYS_TYPES_H 1
#endif

/* Define to 1 if the system has the type `uint16_t'. */
#ifndef HAVE_UINT16_T
#cmakedefine HAVE_UINT16_T 1
#endif

/* Define to 1 if the system has the type `uint32_t'. */
#ifndef HAVE_UINT32_T
#cmakedefine HAVE_UINT32_T 1
#endif

/* Define to 1 if the system has the type `uint64_t'. */
#ifndef HAVE_UINT64_T
#cmakedefine HAVE_UINT64_T 1
#endif

/* Define to 1 if the system has the type `uint8_t'. */
#ifndef HAVE_UINT8_T
#cmakedefine HAVE_UINT8_T 1
#endif

/* Define to 1 if you have the <unistd.h> header file. */
#ifndef HAVE_UNISTD_H
#cmakedefine HAVE_UNISTD_H 1
#endif

/* Define to 1 if you have the <codecvt> header file. */
#ifndef HAVE_CODECVT
#cmakedefine HAVE_CODECVT 1
#endif

/* Define to 1 if the system has the type `unsigned long long'. */
#ifndef HAVE_UNSIGNED_LONG_LONG
#cmakedefine HAVE_UNSIGNED_LONG_LONG 1
#endif

/* Define to 1 or 0, depending whether the compiler supports simple visibility
   declarations. */
#ifndef HAVE_VISIBILITY
#cmakedefine HAVE_VISIBILITY 1
#endif

/* int16_t is available */
#ifndef OSMSCOUT_HAVE_INT16_T
#cmakedefine OSMSCOUT_HAVE_INT16_T 1
#endif

/* int32_t is available */
#ifndef OSMSCOUT_HAVE_INT32_T
#cmakedefine OSMSCOUT_HAVE_INT32_T 1
#endif

/* int64_t is available */
#ifndef OSMSCOUT_HAVE_INT64_T
#cmakedefine OSMSCOUT_HAVE_INT64_T 1
#endif

/* int8_t is available */
#ifndef OSMSCOUT_HAVE_INT8_T
#cmakedefine OSMSCOUT_HAVE_INT8_T 1
#endif

/* libmarisa detected */
#ifndef OSMSCOUT_HAVE_LIB_MARISA
#cmakedefine OSMSCOUT_HAVE_LIB_MARISA 1
#endif

/* long long is available */
#ifndef OSMSCOUT_HAVE_LONG_LONG
#cmakedefine OSMSCOUT_HAVE_LONG_LONG 1
#endif

/* SSE2 processor extension available */
#ifndef OSMSCOUT_HAVE_SSE2
#cmakedefine OSMSCOUT_HAVE_SSE2 1
#endif

/* system header <stdint.h> is available */
#ifndef OSMSCOUT_HAVE_STDINT_H
#cmakedefine OSMSCOUT_HAVE_STDINT_H 1
#endif

/* std::wstring is available */
#ifndef OSMSCOUT_HAVE_STD_WSTRING
#cmakedefine OSMSCOUT_HAVE_STD_WSTRING 1
#endif

/* uint16_t is available */
#ifndef OSMSCOUT_HAVE_UINT16_T
#cmakedefine OSMSCOUT_HAVE_UINT16_T 1
#endif

/* uint32_t is available */
#ifndef OSMSCOUT_HAVE_UINT32_T
#cmakedefine OSMSCOUT_HAVE_UINT32_T 1
#endif

/* uint64_t is available */
#ifndef OSMSCOUT_HAVE_UINT64_T
#cmakedefine OSMSCOUT_HAVE_UINT64_T 1
#endif

/* uint8_t is available */
#ifndef OSMSCOUT_HAVE_UINT8_T
#cmakedefine OSMSCOUT_HAVE_UINT8_T 1
#endif

/* unsigned long long is available */
#ifndef OSMSCOUT_HAVE_ULONG_LONG
#cmakedefine OSMSCOUT_HAVE_ULONG_LONG 1
#endif

/* The size of `wchar_t', as computed by sizeof. */
#ifndef SIZEOF_WCHAR_T
#cmakedefine SIZEOF_WCHAR_T @SIZEOF_WCHAR_T@
#endif

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#ifndef LT_OBJDIR
#define LT_OBJDIR ".libs/"
#endif

/* Define to the address where bug reports for this package should be sent. */
#ifndef PACKAGE_BUGREPORT
#define PACKAGE_BUGREPORT "tim@teulings.org"
#endif

/* Define to the full name of this package. */
#ifndef PACKAGE_NAME
#define PACKAGE_NAME "@name@"
#endif

/* Define to the full name and version of this package. */
#ifndef PACKAGE_STRING
#define PACKAGE_STRING "@name@ 0.1"
#endif

/* Define to the one symbol short name of this package. */
#ifndef PACKAGE_TARNAME
#define PACKAGE_TARNAME "@name@"
#endif

/* Define to the home page for this package. */
#ifndef PACKAGE_URL
#define PACKAGE_URL ""
#endif

/* Define to the version of this package. */
#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "0.1"
#endif

/* Define to 1 if you have the ANSI C header files. */
#ifndef STDC_HEADERS
#cmakedefine STDC_HEADERS 1
#endif

/* Define to 1 if you have OpenMP. */
#ifndef OSMSCOUT_HAVE_OPENMP
#cmakedefine OSMSCOUT_HAVE_OPENMP 1
#endif

/* libmarisa detected */
#ifndef OSMSCOUT_IMPORT_HAVE_LIB_MARISA
#cmakedefine OSMSCOUT_IMPORT_HAVE_LIB_MARISA 1
#endif

/* libprotobuf detected */
#ifndef HAVE_LIB_PROTOBUF
#cmakedefine HAVE_LIB_PROTOBUF 1
#endif

/* libxml detected */
#ifndef HAVE_LIB_XML
#cmakedefine HAVE_LIB_XML 1
#endif

/* zlib detected */
#ifndef HAVE_LIB_ZLIB
#cmakedefine HAVE_LIB_ZLIB 1
#endif

/* iconv detected */
#ifndef HAVE_ICONV
#cmakedefine HAVE_ICONV 1
#endif
#ifndef ICONV_CONST
#define ICONV_CONST @ICONV_CONST@
#endif

/* libagg detected */
#ifndef HAVE_LIB_AGG
#cmakedefine HAVE_LIB_AGG 1
#endif

/* freetype detected */
#ifndef HAVE_LIB_FREETYPE
#cmakedefine HAVE_LIB_FREETYPE 1
#endif

/* cairo detected */
#ifndef HAVE_LIB_CAIRO
#cmakedefine HAVE_LIB_CAIRO 1
#endif

/* pango detected */
#ifndef HAVE_LIB_PANGO
#cmakedefine HAVE_LIB_PANGO 1
#endif

/* pango detected */
#ifndef OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO
#cmakedefine OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO 1
#endif

/* pango detected */
#ifndef OSMSCOUT_MAP_SVG_HAVE_LIB_PANGO
#cmakedefine OSMSCOUT_MAP_SVG_HAVE_LIB_PANGO 1
#endif

/* gl/glut */
#ifndef OSMSCOUT_MAP_OPENGL_HAVE_GL_GLUT_H
#cmakedefine OSMSCOUT_MAP_OPENGL_HAVE_GL_GLUT_H 1
#endif

/* glut/glut */
#ifndef OSMSCOUT_MAP_OPENGL_HAVE_GLUT_GLUT_H
#cmakedefine OSMSCOUT_MAP_OPENGL_HAVE_GLUT_GLUT_H 1
#endif

#endif // @OSMSCOUT_PRIVATE_CONFIG_HEADER_NAME@_PRIVATE_CONFIG_H
