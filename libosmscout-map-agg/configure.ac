dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.56)

AC_INIT([libosmscout-map-agg],[0.1],[tim@teulings.org],[libosmscout-map-agg])
AM_INIT_AUTOMAKE([no-define nostdinc subdir-objects dist-bzip2])

AC_CONFIG_SRCDIR(src/osmscout/MapPainterAgg.cpp)
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([include/osmscout/private/Config.h include/osmscout/MapAggFeatures.h])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_PROG_CXX
AC_PROG_INSTALL

LT_INIT([win32-dll disable-static])
gl_VISIBILITY
AS_IF([test "$HAVE_VISIBILITY" == "1"],
      [AC_DEFINE([OSMSCOUT_MAP_AGG_EXPORT_SYMBOLS],[1],[libosmscout uses special gcc compiler features to export symbols])])

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

PKG_CHECK_MODULES(LIBFREETYPE, [freetype2],
                  HASLIBFREETYPE=true
                  [AC_SUBST(LIBFREETYPE_CFLAGS)
                   AC_SUBST(LIBFREETYPE_LIBS)
                   AC_DEFINE(HAVE_LIB_FREETYPE,1,[libfreetype found])],
                   [HASFREETYPE=false])

AM_CONDITIONAL(HAVE_LIB_FREETYPE,[test "$HASLIBFREETYPE" = "true"])

PKG_CHECK_MODULES(LIBAGG, [libagg],
                  [if test "$HASLIBFREETYPE" = "true"; then
                     LIBAGG_LIBS="-laggfontfreetype $LIBAGG_LIBS"
                   fi
                   AC_SUBST(LIBAGG_CFLAGS)
                   AC_SUBST(LIBAGG_LIBS)])

CPPFLAGS="-DLIB_DATADIR=\\\"$datadir/$PACKAGE_NAME\\\" $CPPFLAGS"

AX_CREATE_PKGCONFIG_INFO([],
                         [libosmscout libosmscout-map],
                         [-losmscoutmapagg $LIBAGG_LIBS $LIBFREETYPE_LIBS],
                         [libosmscout map rendering agg backend library],
                         [$LIBAGG_CFLAGS $LIBFREETYPE_CFLAGS],
                         [])

AC_CONFIG_FILES([Makefile src/Makefile include/Makefile])
AC_OUTPUT
