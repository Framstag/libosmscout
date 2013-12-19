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
LT_INIT([pic-only disable-static win32-dll])
AC_PROG_INSTALL

AX_CXXFLAGS_WARN_ALL

AC_TYPE_SIZE_T

AC_SEARCH_LIBS([sqrt],[m],[])

if test "$GXX" = yes; then
  CXXFLAGS="$CXXFLAGS -Wpointer-arith -Wsign-compare -Wundef -Wcast-qual -Wcast-align -Wwrite-strings -Wredundant-decls -Wno-long-long"
fi

dnl Selfmade tests

dnl AC_MSG_CHECKING([for gcc symbol visibility support])
dnl OLDCXXFLAGS="$CXXFLAGS"
dnl CXXFLAGS="$CXXFLAGS -fvisibility=hidden -fvisibility-inlines-hidden"
dnl AC_TRY_COMPILE([],
dnl                [],
dnl                [LIB_CXXFLAGS="-fvisibility=hidden -fvisibility-inlines-hidden"
dnl                 AC_SUBST(LIB_CXXFLAGS)
dnl                 AC_MSG_RESULT(yes)
dnl                 AC_DEFINE(OSMSCOUT_MAP_QT_EXPORT_SYMBOLS,1,[libosmscout uses special gcc compiler features to export symbols])],
dnl                [AC_MSG_RESULT(no)])
dnl CXXFLAGS="$OLDCXXFLAGS"

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

PKG_CHECK_MODULES(LIBQTGUI, [Qt5Gui >= 5.0.0],
                  [AC_SUBST(LIBQTGUI_CFLAGS)
                   AC_SUBST(LIBQTGUI_LIBS)])

CPPFLAGS="-DLIB_DATADIR=\\\"$datadir/$PACKAGE_NAME\\\" $CPPFLAGS"

AX_CREATE_PKGCONFIG_INFO([],
                         [libosmscout libosmscout-map],
                         [-losmscoutmapqt $LIBQTGUI_LIBS],
                         [libosmscout map rendering Qt backend library],
                         [$LIBQTGUI_CFLAGS],
                         [])

AC_CONFIG_FILES([Makefile src/Makefile include/Makefile])
AC_OUTPUT
