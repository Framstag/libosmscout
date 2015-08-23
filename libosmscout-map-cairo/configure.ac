dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.56)

AC_INIT([libosmscout-map-cairo],[0.1],[tim@teulings.org],[libosmscout-map-cairo])
AM_INIT_AUTOMAKE([no-define nostdinc subdir-objects dist-bzip2])

AC_CONFIG_SRCDIR(src/osmscout/MapPainterCairo.cpp)
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([include/osmscout/private/Config.h include/osmscout/MapCairoFeatures.h])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_PROG_CXX
AC_PROG_INSTALL

LT_INIT([win32-dll])
gl_VISIBILITY
AS_IF([test "$HAVE_VISIBILITY" == "1"],
      [AC_DEFINE([OSMSCOUT_MAP_CAIRO_EXPORT_SYMBOLS],[1],[libosmscout uses special gcc compiler features to export symbols])])

AC_LANG(C++)
AX_CXX_COMPILE_STDCXX_11([],[mandatory])

AX_CXXFLAGS_WARN_ALL
if test "$GXX" = yes; then
  CXXFLAGS="$CXXFLAGS -Wpointer-arith -Wsign-compare -Wundef -Wcast-qual -Wcast-align -Wwrite-strings -Wredundant-decls -Wno-long-long"
fi

AC_SEARCH_LIBS([sqrt],[m],[])

dnl Selfmade tests

AC_MSG_CHECKING([for WIN32])
  AC_LINK_IFELSE([
    AC_LANG_PROGRAM([[#include <windows.h>]],
                    [[GetModuleHandleA(0);]])],
    [AC_MSG_RESULT(yes);
     LDFLAGS="$LDFLAGS -no-undefined"],
    [AC_MSG_RESULT(no)])

dnl Check for libosmscout
PKG_CHECK_MODULES(LIBOSMSCOUT,[libosmscout libosmscout-map],
                  [AC_SUBST(LIBOSMSCOUT_CFLAGS)
                   AC_SUBST(LIBOSMSCOUT_LIBS)])

PKG_CHECK_MODULES(LIBCAIRO, cairo,
                  [AC_SUBST(LIBCAIRO_CFLAGS)
                   AC_SUBST(LIBCAIRO_LIBS)
                   AC_DEFINE(HAVE_LIB_CAIRO,1,[libcairo found])])

PKG_CHECK_MODULES(LIBPANGO, pangocairo,
                  [AC_SUBST(LIBPANGO_CFLAGS)
                   AC_SUBST(LIBPANGO_LIBS)
                   AC_DEFINE(HAVE_LIB_PANGO,1,[libpango found])
                   AC_DEFINE(OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO,1,[libpango found])])

PKG_CHECK_MODULES(LIBPNG,libpng,
                  HASPNG=true
                  [AC_SUBST(LIBPNG_CFLAGS)
                   AC_SUBST(LIBPNG_LIBS)
                   AC_DEFINE(HAVE_LIB_PNG,1,[libpng found])],
                   [HASPNG=false])

AM_CONDITIONAL(HAVE_LIB_PNG,[test "$HASPNG" = "true"])


CPPFLAGS="-DLIB_DATADIR=\\\"$datadir/$PACKAGE_NAME\\\" $CPPFLAGS"

AX_CREATE_PKGCONFIG_INFO([],
                         [libosmscout libosmscout-map],
                         [-losmscoutmapcairo $LIBCAIRO_LIBS $LIBPANGO_LIBS $LIBPNG_LIBS],
                         [libosmscout map rendering cairo backend library],
                         [$LIBCAIRO_CFLAGS $LIBPANGO_CFLAGS $LIBPNG_CFLAGS],
                         [])

AC_CONFIG_FILES([Makefile src/Makefile include/Makefile])
AC_OUTPUT

