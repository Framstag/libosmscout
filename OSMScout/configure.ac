dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.56)
AC_INIT([OSMScout],
        [0.1],
        [Send bugreports to <tim@teulings.org>],
        [OSMScout])

AC_CONFIG_SRCDIR(src/OSMScout.cpp)
AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_HEADERS(src/config.h)
AM_INIT_AUTOMAKE([nostdinc subdir-objects dist-bzip2 no-dist-gzip -Wall -Werror])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_PROG_CXX
AC_LANG(C++)
LT_INIT()

AC_PROG_INSTALL

AX_CXXFLAGS_WARN_ALL

if test "$GXX" = yes; then
  CXXFLAGS="$CXXFLAGS -fPIC -Wpointer-arith -Wsign-compare -Wundef -Wcast-qual -Wcast-align -Wwrite-strings -Wredundant-decls -Wno-long-long"
fi

PKG_CHECK_MODULES(LIBQT,[Qt5Gui >= 5.0.0 Qt5Widgets >= 5.0.0])
AC_SUBST(LIBQT_CFLAGS)
AC_SUBST(LIBQT_LIBS)

PKG_CHECK_MODULES(LIBOSMSCOUT,[libosmscout libosmscout-map libosmscout-map-qt])
AC_SUBST(LIBOSMSCOUT_CFLAGS)
AC_SUBST(LIBOSMSCOUT_LIBS)

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
