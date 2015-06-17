#ifndef OSMSCOUT_IMPORT_PRIVATE_CONFIG_H
#define OSMSCOUT_IMPORT_PRIVATE_CONFIG_H

/* Define to 1 if you have the <dlfcn.h> header file. */
/* #undef HAVE_DLFCN_H */

/* Define to 1 if fseeko (and presumably ftello) exists and is declared. */
#define HAVE_FSEEKO 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* libprotobuf detected */
/* #undef HAVE_LIB_PROTOBUF */

/* libxml detected */
#define HAVE_LIB_XML 1

/* zlib detected */
#define HAVE_LIB_ZLIB 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `mmap' function. */
/* #undef HAVE_MMAP */

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 or 0, depending whether the compiler supports simple visibility
   declarations. */
#define HAVE_VISIBILITY 1

/* libosmscout uses special gcc compiler features to export symbols */
#define OSMSCOUT_IMPORT_EXPORT_SYMBOLS 1

/* libmarisa detected */
/* #undef OSMSCOUT_IMPORT_HAVE_LIB_MARISA */

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "tim@teulings.org"

/* Define to the full name of this package. */
#define PACKAGE_NAME "libosmscout-import"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "libosmscout-import 0.1"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "libosmscout-import"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "0.1"

/* The size of `wchar_t', as computed by sizeof. */
#define SIZEOF_WCHAR_T 4

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

#endif