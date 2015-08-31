dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.56)

AC_INIT([libosmscout],
         [0.1],
         [tim@teulings.org],
         [libosmscout])
AM_INIT_AUTOMAKE([no-define nostdinc subdir-objects dist-bzip2])

AC_CONFIG_SRCDIR(src/osmscout/Database.cpp)
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([include/osmscout/private/Config.h include/osmscout/CoreFeatures.h])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_PROG_CXX
AC_PROG_INSTALL

LT_INIT([win32-dll])
gl_VISIBILITY
AS_IF([test "$HAVE_VISIBILITY" == "1"],
      [AC_DEFINE([OSMSCOUT_EXPORT_SYMBOLS],[1],[libosmscout uses special gcc compiler features to export symbols])])

AC_LANG([C++])
AX_CXX_COMPILE_STDCXX_11([],[mandatory])

AC_ARG_ENABLE([see2-support],
              [AS_HELP_STRING([--disable-see2-support],
                              [disable use of SEE2 cpu extensions])],
              [])

AC_ARG_ENABLE([thread-support],
              [AS_HELP_STRING([--disable-thread-support],
                              [disable C++0x thread support])],
              [])

AC_ARG_ENABLE([openmp-support],
              [AS_HELP_STRING([--disable-openmp-support],
                              [disable OpenMP support])],
              [])

AC_ARG_ENABLE([marisa-support],
              [AS_HELP_STRING([--disable-marisa-support],
                              [disable usage of libmarisa])],
              [])


AX_CXXFLAGS_WARN_ALL
AS_IF([test "x$GXX" = xyes],
      [CXXFLAGS="$CXXFLAGS -Wextra -Wpointer-arith -Wundef -Wcast-qual -Wcast-align -Wredundant-decls -Wno-long-long"])

dnl Support for XintXX_T constants

AC_CHECK_TYPES([int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t],[],[AC_MSG_ERROR("required type is not available")])

AC_CHECK_TYPES([long long],[AC_DEFINE([OSMSCOUT_HAVE_LONG_LONG],[1],[long long is available])],,)
AC_CHECK_TYPES([unsigned long long],[AC_DEFINE([OSMSCOUT_HAVE_ULONG_LONG],[1],[unsigned long long is available])],,)

dnl the agg backend needs wstring support

AC_CHECK_TYPES([std::wstring],[AC_DEFINE([OSMSCOUT_HAVE_STD_WSTRING],[1],[std::wstring is available])],,[#include <string>])
AC_CHECK_SIZEOF([wchar_t])

AC_CHECK_HEADERS([fcntl.h sys/time.h])
      
dnl c++ 11 extensions

AC_CHECK_HEADER([chrono],
                [AC_DEFINE([OSMSCOUT_HAVE_CHRONO_H],[1],[system header <chrono> is available])])
      
AS_IF([test "$enable_thread_support" != "no"],
      [AC_CHECK_HEADER([thread],
                       [AC_DEFINE([OSMSCOUT_HAVE_THREAD],[1],[system header <thread> is available])])])


AC_MSG_CHECKING([if C++ include <atomic> is usable])
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM(
    [[#include <atomic>]],
    [[std::atomic_bool atomBoolValue;]])],
  [AC_MSG_RESULT(yes)
   AC_DEFINE([OSMSCOUT_HAVE_ATOMIC],[1],[standard library has support for atomic])],
  [AC_MSG_RESULT(no)])

AC_MSG_CHECKING([if C++ include <mutex> is usable])
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM(
    [[#include <mutex>]],
    [[std::mutex mutexValue;]])],
  [AC_MSG_RESULT(yes)
   AC_DEFINE([OSMSCOUT_HAVE_MUTEX],[1],[standard library has support for mutex])],
  [AC_MSG_RESULT(no)])

AC_SEARCH_LIBS([sqrt],[m],[])

AC_MSG_CHECKING([if C++ include <cassert> defines assert() without namespace])
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM(
    [[#include <cassert>]],
    [[assert(true);]])],
  [AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)
   AC_DEFINE([OSMSCOUT_REQUIRES_ASSERTH],[1],[libosmscout needs to include <assert.h>])])

AC_MSG_CHECKING([if C++ include <cmath> defines C math symbols, too])
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM(
    [[#include <cmath>]],
     [[sin(1);]])],
  [AC_MSG_RESULT(yes)
   AC_DEFINE([OSMSCOUT_REQUIRES_MATHH],[1],[libosmscout needs to include <math.h>])],
  [AC_MSG_RESULT(no)])

AC_CHECK_DECL([log2(double)],
              [AC_DEFINE([OSMSCOUT_HAVE_LOG2],[1],[math function log2(double) is available])],
              [],
              [#include <cmath>
               #if defined(OSMSCOUT_REQUIRES_MATHH)
                 #include <math.h>
               #endif
              ])

AC_CHECK_DECL([atanh(double)],
              [AC_DEFINE([OSMSCOUT_HAVE_ATANH],[1],[math function atanh(double) is available])],
              [],
              [#include <cmath>
               #if defined(OSMSCOUT_REQUIRES_MATHH)
                 #include <math.h>
               #endif
              ])

AC_CHECK_DECL([lround(double)],
              [AC_DEFINE([OSMSCOUT_HAVE_LROUND],[1],[math function lround(double) is available])],
              [],
              [#include <cmath>
               #if defined(OSMSCOUT_REQUIRES_MATHH)
                 #include <math.h>
               #endif
              ])

AC_CHECK_FUNCS([mmap posix_fadvise posix_madvise])

AC_SYS_LARGEFILE
AC_FUNC_FSEEKO

dnl Check for SSE2 & Co.

AS_IF([test "$enable_see2_support" != "no"],
      [AX_EXT])

AS_IF([test "$enable_openmp_support" != "no"],
      [AX_OPENMP([AC_SUBST(OPENMP_CXXFLAGS)])])

dnl Selfmade tests

AS_IF([test "x$ac_cv_header_stdint_h" = xyes],
      [AC_DEFINE([OSMSCOUT_HAVE_STDINT_H],[1],[system header <stdint.h> is available])])

dnl SSE support

AS_IF([test "x$ax_cv_have_sse2_ext" = xyes],
      [AC_DEFINE([OSMSCOUT_HAVE_SSE2],[1],[SSE2 processor extension available])])

AM_CONDITIONAL(OSMSCOUT_HAVE_SSE2,[test "x$ax_cv_have_sse2_ext" = xyes])

AS_IF([test "$enable_marisa_support" != "no"],
      [PKG_CHECK_MODULES(MARISA,
                         [marisa],
                         [AC_SUBST(MARISA_CFLAGS)
                          AC_SUBST(MARISA_LIBS)
                          AC_DEFINE(OSMSCOUT_HAVE_LIB_MARISA,1,[libmarisa detected])
                          LIB_MARISA_FOUND=true],
                         [LIB_MARISA_FOUND=false])])

AM_CONDITIONAL(OSMSCOUT_HAVE_LIB_MARISA,[test "$LIB_MARISA_FOUND" = true])

CPPFLAGS="-DLIB_DATADIR=\\\"$datadir/$PACKAGE_NAME\\\" $CPPFLAGS"

AX_CREATE_PKGCONFIG_INFO([],
                         [],
                         [-losmscout $MARISA_LIBS],
                         [libosmscout base library],
                         [$OPENMP_CXXFLAGS $SIMD_FLAGS $MARISA_CFLAGS],
                         [$OPENMP_CXXFLAGS])

AC_CONFIG_FILES([Makefile src/Makefile include/Makefile tests/Makefile])
AC_OUTPUT

