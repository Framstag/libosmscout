dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.56)
AC_INIT(libosmscout-demos,
        [0.1],
        [tim@teulings.org],
        [libosmscout-demos])

AC_CONFIG_SRCDIR(src/Routing.cpp)

AC_CONFIG_HEADERS(src/config.h)
AM_INIT_AUTOMAKE([nostdinc subdir-objects dist-bzip2 no-dist-gzip])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AC_PROG_CXX
AC_PROG_LIBTOOL
AC_PROG_INSTALL

if test "$GXX" = yes; then
  CPPFLAGS="$CPPFLAGS -Wall -pedantic -Wpointer-arith -Wsign-compare -Wundef -Wcast-qual -Wcast-align -Wwrite-strings -Wredundant-decls"
fi

PKG_CHECK_MODULES(LIBOSMSCOUT,[libosmscout])
AC_SUBST(LIBOSMSCOUT_CFLAGS)
AC_SUBST(LIBOSMSCOUT_LIBS)

PKG_CHECK_MODULES(LIBOSMSCOUTMAP,[libosmscout-map])
AC_SUBST(LIBOSMSCOUTMAP_CFLAGS)
AC_SUBST(LIBOSMSCOUTMAP_LIBS)

PKG_CHECK_MODULES(LIBOSMSCOUTMAPSVG,[libosmscout-map-svg],
                  [HASLIBOSMSCOUTMAPSVG=true
                   AC_SUBST(LIBOSMSCOUTMAPSVG_CFLAGS)
                   AC_SUBST(LIBOSMSCOUTMAPSVG_LIBS)
                   AC_DEFINE(HAV_LIB_OSMSCOUTMAPSVG,1,[libosmscout-map-svg found])],
                  [HASLIBOSMSCOUTMAPSVG=false])

AM_CONDITIONAL(HAVE_LIB_OSMSCOUTMAPSVG,[test "$HASLIBOSMSCOUTMAPSVG" = "true"])

PKG_CHECK_MODULES(LIBCAIRO, cairo,
                  HASCAIRO=true
                  [AC_SUBST(LIBCAIRO_CFLAGS)
                   AC_SUBST(LIBCAIRO_LIBS)
                   AC_DEFINE(HAVE_LIB_CAIRO,1,[libcairo found])],
                   [HASCAIRO=false])

AM_CONDITIONAL(HAVE_LIB_CAIRO,[test "$HASCAIRO" = "true"])

PKG_CHECK_MODULES(LIBQTGUI, QtGui > 4.6.0,
                  HASQTGUI=true
                  [AC_SUBST(LIBQTGUI_CFLAGS)
                   AC_SUBST(LIBQTGUI_LIBS)
                   AC_DEFINE(HAVE_LIB_QTGUI,1,[libQtGui found])],
                   [HASQTGUI=false])

AM_CONDITIONAL(HAVE_LIB_QTGUI,[test "$HASQTGUI" = "true"])

PKG_CHECK_MODULES(LIBAGG, libagg,
                  HASAGG=true
                  [AC_SUBST(LIBAGG_CFLAGS)
                   AC_SUBST(LIBAGG_LIBS)
                   AC_DEFINE(HAVE_LIB_AGG,1,[libagg found])],
                   [HASAGG=false])

AM_CONDITIONAL(HAVE_LIB_AGG,[test "$HASAGG" = "true"])

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
