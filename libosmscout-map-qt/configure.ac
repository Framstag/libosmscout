dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.56)
AC_INIT([libosmscout-map-qt],[0.1],[tim@teulings.org],[libosmscout-map-qt])

AC_CONFIG_SRCDIR(src/osmscout/MapPainterQt.cpp)
AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_HEADERS([include/osmscout/private/Config.h include/osmscout/MapQtFeatures.h])
AM_INIT_AUTOMAKE([no-define nostdinc subdir-objects dist-bzip2])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_PROG_CXX
AC_LANG(C++)
LT_INIT([win32-dll])
AC_PROG_INSTALL

AC_TYPE_SIZE_T

AC_SEARCH_LIBS([sqrt],[m])

AC_MSG_CHECKING([if C++ include <cmath> defines C math symbols, too])
AC_TRY_COMPILE([#include <cmath>],
               [sin(1);],
               [AC_MSG_RESULT(yes)],
               [AC_DEFINE([OSMSCOUT_MAP_QT_REQUIRES_MATHH],[1],[libosmscout needs to include <math.h>])
                AC_MSG_RESULT(no)])

AC_CHECK_DECLS([log2(double), atanh(double), lround(double)],
               [],
               [],
               [#include <cmath>
                #if defined(OSMSCOUT_REQUIRES_MATHH)
                  #include <math.h>
                #endif])

if test "x$ac_cv_have_decl_log2_double_" = xyes; then
  AC_DEFINE([OSMSCOUT_MAP_QT_HAVE_LOG2],[1],[math function log2(double) is available])
fi

if test "x$ac_cv_have_decl_atanh_double_" = xyes; then
  AC_DEFINE([OSMSCOUT_MAP_QT_HAVE_ATANH],[1],[math function atanh(double) is available])
fi

if test "x$ac_cv_have_decl_lround_double_" = xyes; then
  AC_DEFINE([OSMSCOUT_MAP_QT_HAVE_LROUND],[1],[math function lround(double) is available])
fi

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
                AC_DEFINE(OSMSCOUT_MAP_QT_EXPORT_SYMBOLS,1,[libosmscout uses special gcc compiler features to export symbols])],
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

PKG_CHECK_MODULES(LIBQTGUI, [QtGui > 4.6.0],
                  [AC_SUBST(LIBQTGUI_CFLAGS)
                   AC_SUBST(LIBQTGUI_LIBS)])

CPPFLAGS="-DLIB_DATADIR=\\\"$datadir/$PACKAGE_NAME\\\" $CPPFLAGS"

AC_CONFIG_FILES([Makefile src/Makefile include/Makefile libosmscout-map-qt.pc libosmscout-map-qt-uninstalled.pc])
AC_OUTPUT
