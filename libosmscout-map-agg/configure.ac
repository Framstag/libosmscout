dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.56)
AC_INIT([libosmscout-map-agg],[0.1],[tim@teulings.org],[libosmscout-map-agg])

AC_CONFIG_SRCDIR(src/osmscout/MapPainterAgg.cpp)
AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_HEADERS([include/osmscout/private/Config.h include/osmscout/MapAggFeatures.h])
AM_INIT_AUTOMAKE([no-define nostdinc subdir-objects dist-bzip2])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_PROG_CXX
AC_LANG(C++)
LT_INIT([win32-dll])
AC_PROG_INSTALL

AX_CHECK_COMPILE_FLAG([-std=c++0x],
                      [CPPFLAGS="$CPPFLAGS -std=c++0x"])

AC_TYPE_SIZE_T

AC_SEARCH_LIBS([sqrt],[m])

AC_CHECK_DECLS([log2(double), atanh(double)],[],[],[#include <cmath>])

if test "$GXX" = yes; then
  CPPFLAGS="$CPPFLAGS -Wall -Wpointer-arith -Wsign-compare -Wundef -Wcast-qual -Wcast-align -Wwrite-strings -Wredundant-decls -Wno-long-long"
fi

dnl Selfmade tests

AC_MSG_CHECKING([for gcc symbol visibility support])
OLDCXXFLAGS="$CXXFLAGS"
CXXFLAGS="$CXXFLAGS -fvisibility=hidden -fvisibility-inlines-hidden"
AC_TRY_COMPILE([],
               [],
               [LIB_CXXFLAGS="-fvisibility=hidden -fvisibility-inlines-hidden"
                AC_SUBST(LIB_CXXFLAGS)
                AC_MSG_RESULT(yes)
                AC_DEFINE(OSMSCOUT_MAP_AGG_EXPORT_SYMBOLS,1,[libosmscout uses special gcc compiler features to export symbols])],
               [AC_MSG_RESULT(no)])
CXXFLAGS="$OLDCXXFLAGS"

AC_MSG_CHECKING([for WIN32])
  AC_LINK_IFELSE([
    AC_LANG_PROGRAM([[#include <windows.h>]],
                    [[GetModuleHandleA(0);]])],
    [AC_MSG_RESULT(yes);
     LDFLAGS="$LDFLAGS -no-undefined"],
    [AC_MSG_RESULT(no)])

dnl Check for libosmscout
PKG_CHECK_MODULES(LIBOSMSCOUT,[libosmscout],
                  [AC_SUBST(LIBOSMSCOUT_CFLAGS)
                   AC_SUBST(LIBOSMSCOUT_LIBS)])

dnl Check for libosmscout-map
PKG_CHECK_MODULES(LIBOSMSCOUTMAP,[libosmscout-map],
                  [AC_SUBST(LIBOSMSCOUTMAP_CFLAGS)
                   AC_SUBST(LIBOSMSCOUTMAP_LIBS)])

PKG_CHECK_MODULES(LIBFREETYPE, [freetype2],
                  HASLIBFREETYPE=true
                  [AC_SUBST(LIBFREETYPE_CFLAGS)
                   AC_SUBST(LIBFREETYPE_LIBS)
                   AC_DEFINE(HAVE_LIB_FREETYPE,1,[libfreetype found])],
                   [HASFREETYPE=false])

AM_CONDITIONAL(HAVE_LIB_FREETYPE,[test "$HASLIBFREETYPE" = "true"])
	
PKG_CHECK_MODULES(LIBAGG, [libagg],
                  [if test "$HASLIBFREETYPE" = "true"; then
                     LIBAGG_LIBS="-laggfontfreetype_pic $LIBAGG_LIBS"
                   fi
                   AC_SUBST(LIBAGG_CFLAGS)
                   AC_SUBST(LIBAGG_LIBS)])

CPPFLAGS="-DLIB_DATADIR=\\\"$datadir/$PACKAGE_NAME\\\" $CPPFLAGS"

AC_CONFIG_FILES([Makefile src/Makefile include/Makefile libosmscout-map-agg.pc libosmscout-map-agg-uninstalled.pc])
AC_OUTPUT
