dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.56)
AC_INIT([OSMScout],
        [0.1],
        [Send bugreports to <tim@teulings.org>],
        [OSMScout])

AC_CONFIG_SRCDIR(src/OSMScout.cpp)
AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_HEADERS(src/config.h)
AM_INIT_AUTOMAKE([nostdinc subdir-objects dist-bzip2 no-dist-gzip])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_PROG_CXX
AC_PROG_LIBTOOL
AC_PROG_INSTALL

AX_CHECK_COMPILE_FLAG([-std=c++0x],
                      [CPPFLAGS="$CPPFLAGS -std=c++0x"])

if test "$GXX" = yes; then
  CPPFLAGS="$CPPFLAGS -Wall -pedantic -Wpointer-arith -Wsign-compare -Wundef -Wcast-qual -Wcast-align -Wwrite-strings -Wredundant-decls -Wno-long-long"
fi

PKG_CHECK_MODULES(LIBQTGUI,[QtGui > 4.6.0])
AC_SUBST(LIBQTGUI_CFLAGS)
AC_SUBST(LIBQTGUI_LIBS)

PKG_CHECK_MODULES(LIBQTOPENGL,[QtOpenGL > 4.6.0],
                  [HAS_LIB_QTOPENGL=true
                   AC_SUBST(LIBQTOPENGL_CFLAGS)
                   AC_SUBST(LIBQTOPENGL_LIBS)
                   AC_DEFINE(HAVE_LIB_QTOPENGL,1,[OpenGL support for Qt found])],
                   [HAS_LIB_QTOPENGL=false])

AM_CONDITIONAL(HAVE_LIB_QTOPENGL,[test "$HAS_LIB_QTOPENGL" = "true"])

PKG_CHECK_MODULES(LIBOSMSCOUT,[libosmscout])
AC_SUBST(LIBOSMSCOUT_CFLAGS)
AC_SUBST(LIBOSMSCOUT_LIBS)

PKG_CHECK_MODULES(LIBOSMSCOUTMAP,[libosmscout-map])
AC_SUBST(LIBOSMSCOUTMAP_CFLAGS)
AC_SUBST(LIBOSMSCOUTMAP_LIBS)

PKG_CHECK_MODULES(LIBOSMSCOUTMAPQT,[libosmscout-map-qt])
AC_SUBST(LIBOSMSCOUTMAPQT_CFLAGS)
AC_SUBST(LIBOSMSCOUTMAPQT_LIBS)

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
