dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.56)
AC_INIT([libosmscout-map],[0.1],[tim@teulings.org],[libosmscout-map])

AC_CONFIG_SRCDIR(src/osmscout/MapPainter.cpp)
AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_HEADERS([include/osmscout/private/Config.h include/osmscout/MapFeatures.h])
AM_INIT_AUTOMAKE([no-define nostdinc subdir-objects dist-bzip2])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AC_PROG_CXX
AC_LANG(C++)
LT_INIT([win32-dll])
AC_PROG_INSTALL

AC_ARG_ENABLE(libagg-driver,
              [  --disable-libagg-driver do not include libagg driver])
AC_ARG_ENABLE(cairo-driver,
              [  --disable-cairo-driver  do not include cairo driver])
AC_ARG_ENABLE(qt-driver,
              [  --disable-qt-driver     do not include Qt driver])
              
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
                AC_DEFINE(OSMSCOUT_MAP_EXPORT_SYMBOLS,1,[libosmscout uses special gcc compiler features to export symbols])],
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
                   
if test "$enable_cairo_driver" != "no"; then
	PKG_CHECK_MODULES(LIBCAIRO, cairo,
	                  HASCAIRO=true
	                  [AC_SUBST(LIBCAIRO_CFLAGS)
	                   AC_SUBST(LIBCAIRO_LIBS)
	                   AC_DEFINE(HAVE_LIB_CAIRO,1,[libcairo found])],
	                   [HASCAIRO=false])
fi
AM_CONDITIONAL(HAVE_LIB_CAIRO,[test "$HASCAIRO" = "true"])

if test "$enable_cairo_driver" != "no"; then
	PKG_CHECK_MODULES(LIBPNG,libpng,
	                  HASPNG=true
	                  [AC_SUBST(LIBPNG_CFLAGS)
                       AC_SUBST(LIBPNG_LIBS)
                       AC_DEFINE(HAVE_LIB_PNG,1,[libpng found])],
                       [HASPNG=false])
fi
AM_CONDITIONAL(HAVE_LIB_PNG,[test "$HASPNG" = "true"])

if test "$enable_qt_driver" != "no"; then
	PKG_CHECK_MODULES(LIBQTGUI, QtGui > 4.6.0,
	                  HASQTGUI=true
	                  [AC_SUBST(LIBQTGUI_CFLAGS)
	                   AC_SUBST(LIBQTGUI_LIBS)
	                   AC_DEFINE(HAVE_LIB_QTGUI,1,[libQtGui found])],
	                   [HASQTGUI=false])
fi
AM_CONDITIONAL(HAVE_LIB_QTGUI,[test "$HASQTGUI" = "true"])

if test "$enable_libagg_driver" != "no"; then
	PKG_CHECK_MODULES(LIBFREETYPE, freetype2,
	                  HASLIBFREETYPE=true
	                  [AC_SUBST(LIBFREETYPE_CFLAGS)
	                   AC_SUBST(LIBFREETYPE_LIBS)
	                   AC_DEFINE(HAVE_LIB_FREETYPE,1,[libfreetype found])],
	                   [HASQTGUI=false])
	
fi
AM_CONDITIONAL(HAVE_LIB_FREETYPE,[test "$HASLIBFREETYPE" = "true"])
	
if test "$enable_libagg_driver" != "no"; then
	PKG_CHECK_MODULES(LIBAGG, libagg,
	                  HASAGG=true
	                  [AC_SUBST(LIBAGG_CFLAGS)
	                   if test "$HASLIBFREETYPE" = "true"; then
	                     LIBAGG_LIBS="-laggfontfreetype_pic $LIBAGG_LIBS"
	                   fi
	                   AC_SUBST(LIBAGG_LIBS)
	                   AC_DEFINE(HAVE_LIB_AGG,1,[libagg found])],
	                   [HASAGG=false])
fi
AM_CONDITIONAL(HAVE_LIB_AGG,[test "$HASAGG" = "true"])

CPPFLAGS="-DLIB_DATADIR=\\\"$datadir/$PACKAGE_NAME\\\" $CPPFLAGS"

AC_CONFIG_FILES([Makefile src/Makefile include/Makefile libosmscout-map.pc libosmscout-map-uninstalled.pc])
AC_OUTPUT

