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
AS_IF([test "x$GXX" = xyes],
      [CXXFLAGS="$CXXFLAGS -Wextra -Wpointer-arith -Wundef -Wcast-qual -Wcast-align -Wredundant-decls -Wno-long-long -Wunused-variable"])

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
                   AC_SUBST(LIBFREETYPE_LIBS)])

AM_CONDITIONAL(HAVE_LIB_FREETYPE,[test "$HASLIBFREETYPE" = "true"])

PKG_CHECK_MODULES(LIBAGG, [libagg],
                  [AC_SUBST(LIBAGG_CFLAGS)
                   AC_SUBST(LIBAGG_LIBS)])

dnl The name dof the agg freetype support library differs between Linux distributions
dnl Since the library is not returned by pkg-config, we have to find the correct
dnl name ourself :-/
SAVED_CXXFLAGS=$CXXFLAGS
SAVED_LDFLAGS=$LDFLAGS
SAVED_LIBS=$LIBS

CXXFLAGS="$SAVED_CXXFLAGS $LIBAGG_CFLAGS $LIBFREETYPE_CFLAGS"
LDFLAGS="$SAVED_LDFLAGS $LIBAGG_LDFLAGS $LIBFREETYPE_LDFLAGS"

HASAGGFREETYPE="false"

AC_MSG_CHECKING([for libagg support library 'aggfontfreetype_pic'])
LIBS="$SAVED_LIBS -laggfontfreetype_pic $LIBAGG_LIBS $LIBFREETYPE_LIBS"

AC_LINK_IFELSE(
  [AC_LANG_PROGRAM([#include <agg2/agg_font_freetype.h>],
                   [ agg::font_engine_freetype_int32 fontEngine;])],
  [AC_MSG_RESULT([yes])
   LIBAGG_LIBS="-laggfontfreetype_pic $LIBAGG_LIBS"
   HASAGGFREETYPE=true],
  [AC_MSG_RESULT([no])])

if test "$HASAGGFREETYPE" == "false"; then

  AC_MSG_CHECKING([for libagg support library 'aggfontfreetype'])
  LIBS="$SAVED_LIBS -laggfontfreetype $LIBAGG_LIBS $LIBFREETYPE_LIBS"
  AC_LINK_IFELSE(
    [AC_LANG_PROGRAM([#include <agg2/agg_font_freetype.h>],
                     [ agg::font_engine_freetype_int32 fontEngine;])],
    [AC_MSG_RESULT([yes])
     LIBAGG_LIBS="-laggfontfreetype $LIBAGG_LIBS"
     HASAGGFREETYPE=true],
    [AC_MSG_RESULT([no])])
fi

AC_MSG_CHECKING([for Agg compilation])
  AC_LINK_IFELSE(
    [AC_LANG_PROGRAM(
[[
#include <mutex>
#include <agg2/agg_conv_curve.h>
#include <agg2/agg_conv_contour.h>
#include <agg2/agg_path_storage.h>
#include <agg2/agg_pixfmt_rgb.h>
#include <agg2/agg_rasterizer_scanline_aa.h>
#include <agg2/agg_renderer_base.h>
#include <agg2/agg_rendering_buffer.h>
#include <agg2/agg_renderer_scanline.h>
#include <agg2/agg_scanline_p.h>
#include <agg2/agg_scanline_u.h>
#include <agg2/agg_conv_bspline.h>
#include <agg2/agg_conv_dash.h>
#include <agg2/agg_conv_segmentator.h>
#include <agg2/agg_conv_stroke.h>
#include <agg2/agg_ellipse.h>
#include <agg2/agg_trans_single_path.h>
#include <iostream>
#include <limits>
#include <agg2/agg_conv_bspline.h>
#include <agg2/agg_conv_dash.h>
#include <agg2/agg_conv_segmentator.h>
#include <agg2/agg_conv_stroke.h>
#include <agg2/agg_ellipse.h>
#include <agg2/agg_trans_single_path.h>
#include <agg2/agg_font_freetype.h>]],
    [[bool test;]])],
    [AC_MSG_RESULT([yes])
     AGGPASSED=true],
    [AC_MSG_FAILURE([cannot compile simple agg program])])

LIBS=$SAVED_LIBS
LDFLAGS=$SAVED_LDFLAGS
CXXFLAGS=$SAVED_CXXFLAGS

CPPFLAGS="-DLIB_DATADIR=\\\"$datadir/$PACKAGE_NAME\\\" $CPPFLAGS"

AX_CREATE_PKGCONFIG_INFO([],
                         [libosmscout libosmscout-map],
                         [-losmscoutmapagg $LIBAGG_LIBS],
                         [libosmscout map rendering agg backend library],
                         [$LIBAGG_CFLAGS $LIBFREETYPE_CFLAGS],
                         [])

AC_CONFIG_FILES([Makefile src/Makefile include/Makefile])
AC_OUTPUT
