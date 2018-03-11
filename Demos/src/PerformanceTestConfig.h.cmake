#ifndef LIBOSMSCOUT_PERFORMANCETEST_PRIVATE_CONFIG_H
#define LIBOSMSCOUT_PERFORMANCETEST_PRIVATE_CONFIG_H

/* Define to 1 if you have the <dlfcn.h> header file. */
#cmakedefine HAVE_DLFCN_H 1

/* Define to 1 if fseeko (and presumably ftello) exists and is declared. */
#cmakedefine HAVE_FSEEKO 1

/* Define to 1 if you have the <inttypes.h> header file. */
#cmakedefine HAVE_INTTYPES_H 1

/* cairo detected */
#cmakedefine HAVE_LIB_CAIRO 1

#cmakedefine HAVE_LIB_OSMSCOUTMAPCAIRO 1

/* Gperftools detected */
#cmakedefine HAVE_LIB_GPERFTOOLS 1

/* pango detected */
#cmakedefine OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO 1

/* qt detected */
#cmakedefine HAVE_LIB_QT5_GUI 1

#cmakedefine HAVE_LIB_OSMSCOUTMAPQT 1

/* Agg detected */
#cmakedefine HAVE_LIB_AGG 1
#cmakedefine HAVE_LIB_OSMSCOUTMAPAGG 1

/* Opengl detected */
#cmakedefine HAVE_LIB_OPENGL 1
#cmakedefine HAVE_LIB_OSMSCOUTMAPOPENGL 1

/* Define to 1 if you have the <memory.h> header file. */
#cmakedefine HAVE_MEMORY_H 1

/* Define to 1 if you have the `mallinfo' function. */
#cmakedefine HAVE_MALLINFO 1

/* Define to 1 if you have the `mmap' function. */
#cmakedefine HAVE_MMAP 1

/* Define to 1 if you have the <stdint.h> header file. */
#cmakedefine HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#cmakedefine HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#cmakedefine HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#cmakedefine HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#cmakedefine HAVE_SYS_STAT_H */

/* Define to 1 if you have the <sys/types.h> header file. */
#cmakedefine HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#cmakedefine HAVE_UNISTD_H 1

/* Define to 1 or 0, depending whether the compiler supports simple visibility
   declarations. */
#cmakedefine HAVE_VISIBILITY 1

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "tim@teulings.org"

/* Define to the full name of this package. */
#define PACKAGE_NAME "PerformanceTest"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "PerformanceTest 0.1"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "PerformanceTest"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "0.1"

/* The size of `wchar_t', as computed by sizeof. */
#define SIZEOF_WCHAR_T @SIZEOF_WCHAR_T@

/* Define to 1 if you have the ANSI C header files. */
#cmakedefine STDC_HEADERS 1

#endif