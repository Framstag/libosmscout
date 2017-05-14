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

LT_INIT([win32-dll disable-static])
gl_VISIBILITY
AS_IF([test "$HAVE_VISIBILITY" == "1"],
      [AC_DEFINE([OSMSCOUT_EXPORT_SYMBOLS],[1],[libosmscout uses special gcc compiler features to export symbols])])

AC_LANG([C++])
AX_CXX_COMPILE_STDCXX([11],[noext],[mandatory])

AC_ARG_ENABLE([see2-support],
              [AS_HELP_STRING([--disable-see2-support],
                              [disable use of SEE2 cpu extensions])],
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
      [CXXFLAGS="$CXXFLAGS -Wextra -Wpointer-arith -Wundef -Wcast-qual -Wcast-align -Wredundant-decls -Wno-long-long -Wunused-variable"])

dnl some optional headers we handle
AC_CHECK_HEADERS([fcntl.h sys/stat.h codecvt])

dnl should all be available with C++11, but we check it anyway to fail fast
AC_CHECK_TYPES([long long, unsigned long long, int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t],[],[AC_MSG_ERROR("required type is not available")])

dnl Check, if shared_mutex is available
AC_CHECK_TYPES([std::shared_mutex],[AC_DEFINE([OSMSCOUT_HAVE_SHARED_MUTEX],[1],[std::shared_mutex is available])],,[#include <shared_mutex>])

AC_CHECK_SIZEOF([wchar_t])

AC_CHECK_FUNCS([mmap posix_fadvise posix_madvise])

AC_SEARCH_LIBS([sqrt],[m],[])

dnl Activate large file support
AC_SYS_LARGEFILE
AC_FUNC_FSEEKO

dnl Check for SSE2 & Co.

AS_IF([test "$enable_see2_support" != "no"],
      [AX_EXT])

dnl SSE support
AS_IF([test "x$ax_cv_have_sse2_ext" = xyes],
      [AC_DEFINE([OSMSCOUT_HAVE_SSE2],[1],[SSE2 processor extension available])])

AM_CONDITIONAL(OSMSCOUT_HAVE_SSE2,[test "x$ax_cv_have_sse2_ext" = xyes])

AS_IF([test "$enable_openmp_support" != "no"],
      [AX_OPENMP([AC_SUBST(OPENMP_CXXFLAGS)])])

AS_IF([test "$enable_marisa_support" != "no"],
      [PKG_CHECK_MODULES(MARISA,
                         [marisa],
                         [AC_SUBST(MARISA_CFLAGS)
                          AC_SUBST(MARISA_LIBS)
                          AC_DEFINE(OSMSCOUT_HAVE_LIB_MARISA,1,[libmarisa detected])
                          LIB_MARISA_FOUND=true],
                         [LIB_MARISA_FOUND=false])])

AM_CONDITIONAL(OSMSCOUT_HAVE_LIB_MARISA,[test "$LIB_MARISA_FOUND" = true])

AX_PTHREAD

CPPFLAGS="-DLIB_DATADIR=\\\"$datadir/$PACKAGE_NAME\\\" $CPPFLAGS"

AX_CREATE_PKGCONFIG_INFO([],
                         [],
                         [-losmscout $PTHREAD_CFLAGS $PTHREAD_LIBS $MARISA_LIBS],
                         [libosmscout base library],
                         [$PTHREAD_CFLAGS $OPENMP_CXXFLAGS $SIMD_FLAGS $MARISA_CFLAGS],
                         [$OPENMP_CXXFLAGS])

AC_CONFIG_FILES([Makefile src/Makefile include/Makefile])
AC_OUTPUT

