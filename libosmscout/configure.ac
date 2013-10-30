dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.56)
AC_INIT([libosmscout],[0.1],[tim@teulings.org],[libosmscout])

AC_CONFIG_SRCDIR(src/osmscout/Database.cpp)
AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_HEADERS([include/osmscout/private/Config.h include/osmscout/CoreFeatures.h])
AM_INIT_AUTOMAKE([no-define nostdinc subdir-objects dist-bzip2])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_PROG_CXX
AC_LANG(C++)
LT_INIT([win32-dll])
AC_PROG_INSTALL

AC_ARG_ENABLE(see2-support,
              [AS_HELP_STRING([--disable-see2-support],
                              [do not make use of SEE2 cpu extensions])])

AC_ARG_ENABLE(cpp0x-support,
              [AS_HELP_STRING([--disable-cpp0x-support],
                              [do not make use of C++0x compiler support])])

AC_ARG_ENABLE(thread-support,
              [AS_HELP_STRING([--disable-thread-support],
                              [do not make use of C++0x thread support])])

AC_ARG_ENABLE(openmp-support,
              [AS_HELP_STRING([--disable-openmp-support],
                              [do not make use of OpenMP support])])

if test "$enable_cpp0x_support" != "no"; then
 AX_CHECK_COMPILE_FLAG([-std=c++0x],
                       [CPP0XFLAGS="-std=c++0x"
                        CPPFLAGS="$CPPFLAGS -std=c++0x"
                        AC_SUBST(CPP0XFLAGS)])
fi

AC_TYPE_SIZE_T

AC_CHECK_TYPES([std::wstring],,,[#include <string>])

AC_CHECK_TYPES([int8_t],,,[#include <stdint.h>])
AC_CHECK_TYPES([uint8_t],,,[#include <stdint.h>])
AC_CHECK_TYPES([int16_t],,,[#include <stdint.h>])
AC_CHECK_TYPES([uint16_t],,,[#include <stdint.h>])
AC_CHECK_TYPES([int32_t],,,[#include <stdint.h>])
AC_CHECK_TYPES([uint32_t],,,[#include <stdint.h>])
AC_CHECK_TYPES([int64_t],,,[#include <stdint.h>])
AC_CHECK_TYPES([uint64_t],,,[#include <stdint.h>])

AC_CHECK_SIZEOF([wchar_t])

AC_CHECK_HEADERS([fcntl.h sys/time.h])

if test "$enable_cpp0x_support" != "no"; then
AC_CHECK_HEADERS([unordered_map unordered_set])
fi

if test "$enable_thread_support" != "no"; then
AC_CHECK_HEADERS([thread])
fi

AC_SEARCH_LIBS([sqrt],[m])

AC_MSG_CHECKING([if C++ include <cassert> defines assert() without namespace])
AC_TRY_COMPILE([#include <cassert>],
               [assert(true);],
               [AC_MSG_RESULT(yes)],
               [AC_DEFINE([OSMSCOUT_REQUIRES_ASSERTH],[1],[libosmscout needs to include <assert.h>])
                AC_MSG_RESULT(no)])

AC_MSG_CHECKING([if C++ include <cmath> defines C math symbols, too])
AC_TRY_COMPILE([#include <cmath>],
               [sin(1);],
               [AC_MSG_RESULT(yes)],
               [AC_DEFINE([OSMSCOUT_REQUIRES_MATHH],[1],[libosmscout needs to include <math.h>])
                AC_MSG_RESULT(no)])

AC_CHECK_DECLS([log2(double), atanh(double), lround(double)],
               [],
               [],
               [#include <cmath>
                #if defined(OSMSCOUT_REQUIRES_MATHH)
                  #include <math.h>
                #endif])

if test "x$ac_cv_have_decl_log2_double_" = xyes; then
  AC_DEFINE([OSMSCOUT_HAVE_LOG2],[1],[math function log2(double) is available])
fi

if test "x$ac_cv_have_decl_atanh_double_" = xyes; then
  AC_DEFINE([OSMSCOUT_HAVE_ATANH],[1],[math function atanh(double) is available])
fi

if test "x$ac_cv_have_decl_lround_double_" = xyes; then
  AC_DEFINE([OSMSCOUT_HAVE_LROUND],[1],[math function lround(double) is available])
fi

AC_CHECK_FUNCS([mmap posix_fadvise posix_madvise])

AC_SYS_LARGEFILE
AC_FUNC_FSEEKO

dnl Check for SSE2 & Co.

if test "$enable_see2_support" != "no"; then
  AX_EXT
fi

if test "$enable_openmp_support" != "no"; then
  AX_OPENMP([AC_SUBST(OPENMP_CXXFLAGS)])
fi  

if test "x$GXX" = xyes; then
  CPPFLAGS="$CPPFLAGS -Wall -Wpointer-arith -Wsign-compare -Wundef -Wcast-qual -Wcast-align -Wwrite-strings -Wredundant-decls -Wno-long-long"
fi

dnl Selfmade tests

if test "x$ac_cv_header_stdint_h" = xyes; then
  AC_DEFINE([OSMSCOUT_HAVE_STDINT_H],[1],[system header <stdint.h> is available])
fi

if test "x$ac_cv_type_std__wstring" = xyes; then
  AC_DEFINE([OSMSCOUT_HAVE_STD__WSTRING],[1],[std::wstring is available])
fi

dnl Support for XintXX_T constants

if test "x$ac_cv_type_int8_t" = xyes; then
  AC_DEFINE([OSMSCOUT_HAVE_INT8_T],[1],[int8_t is available])
fi

if test "x$ac_cv_type_uint8_t" = xyes; then
  AC_DEFINE([OSMSCOUT_HAVE_UINT8_T],[1],[uint8_t is available])
fi

if test "x$ac_cv_type_int16_t" = xyes; then
  AC_DEFINE([OSMSCOUT_HAVE_INT16_T],[1],[int16_t is available])
fi

if test "x$ac_cv_type_uint16_t" = xyes; then
  AC_DEFINE([OSMSCOUT_HAVE_UINT16_T],[1],[uint16_t is available])
fi

if test "x$ac_cv_type_int32_t" = xyes; then
  AC_DEFINE([OSMSCOUT_HAVE_INT32_T],[1],[int32_t is available])
fi

if test "x$ac_cv_type_uint32_t" = xyes; then
  AC_DEFINE([OSMSCOUT_HAVE_UINT32_T],[1],[uint32_t is available])
fi

if test "x$ac_cv_type_int64_t" = xyes; then
  AC_DEFINE([OSMSCOUT_HAVE_INT64_T],[1],[int64_t is available])
fi

if test "x$ac_cv_type_uint64_t" = xyes; then
  AC_DEFINE([OSMSCOUT_HAVE_UINT64_T],[1],[uint64_t is available])
fi

dnl c++ 11 extensions

if test "x$ac_cv_header_unordered_map" = xyes; then
  AC_DEFINE([OSMSCOUT_HAVE_UNORDERED_MAP],[1],[system header <unordered_map> is available])
fi

if test "x$ac_cv_header_unordered_set" = xyes; then
  AC_DEFINE([OSMSCOUT_HAVE_UNORDERED_SET],[1],[system header <unordered_set> is available])
fi

if test "x$ac_cv_header_thread" = xyes; then
  AC_DEFINE([OSMSCOUT_HAVE_THREAD],[1],[system header <thread> is available])
fi

dnl SSE support

if test "x$ax_cv_have_sse2_ext" = xyes; then
  AC_DEFINE([OSMSCOUT_HAVE_SSE2],[1],[SSE2 processor extension available])
fi

if test "$build_os" != "mingw32"; then
  AC_MSG_CHECKING([for gcc symbol visibility support])
  OLDCXXFLAGS="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS -fvisibility=hidden -fvisibility-inlines-hidden"
  AC_TRY_COMPILE([],
                 [],
                 [LIB_CXXFLAGS="-fvisibility=hidden -fvisibility-inlines-hidden"
                  AC_SUBST(LIB_CXXFLAGS)
                  AC_MSG_RESULT(yes)
                  AC_DEFINE([OSMSCOUT_EXPORT_SYMBOLS],[1],[libosmscout uses special gcc compiler features to export symbols])],
                 [AC_MSG_RESULT(no)])
  CXXFLAGS="$OLDCXXFLAGS"
fi

AM_CONDITIONAL(HAVE_LIB_PROTOBUF,[test "$LIB_PROTOBUF_FOUND" = true])

CPPFLAGS="-DLIB_DATADIR=\\\"$datadir/$PACKAGE_NAME\\\" $CPPFLAGS"

AC_CONFIG_FILES([Makefile src/Makefile include/Makefile tests/Makefile libosmscout.pc libosmscout-uninstalled.pc])
AC_OUTPUT

