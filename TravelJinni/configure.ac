dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.56)
AC_INIT([TravelJinni],
        [0.1],
        [Send bugreports to <tim@teulings.org>],
        [TravelJinni])

AC_CONFIG_SRCDIR(src/Viewer.cpp)

AC_CONFIG_HEADERS(src/config.h)
AM_INIT_AUTOMAKE([nostdinc subdir-objects dist-bzip2 no-dist-gzip])
AC_PROG_CXX
AC_PROG_LIBTOOL
AC_PROG_INSTALL

if test "$GXX" = yes; then
  CPPFLAGS="$CPPFLAGS -Wall -pedantic -Wpointer-arith -Wsign-compare -Wundef -Wcast-qual -Wcast-align -Wwrite-strings -Wredundant-decls -Wno-long-long"
fi

PKG_CHECK_MODULES(LIBILLUMINATION, illumination-full)
AC_SUBST(LIBILLUMINATION_CFLAGS)
AC_SUBST(LIBILLUMINATION_LIBS)

PKG_CHECK_MODULES(LIBOSMSCOUT,[libosmscout])
AC_SUBST(LIBOSMSCOUT_CFLAGS)
AC_SUBST(LIBOSMSCOUT_LIBS)

PKG_CHECK_MODULES(LIBOSMSCOUTMAP,[libosmscout-map])
AC_SUBST(LIBOSMSCOUTMAP_CFLAGS)
AC_SUBST(LIBOSMSCOUTMAP_LIBS)

PKG_CHECK_MODULES(LIBCAIRO, cairo)
AC_SUBST(LIBCAIRO_CFLAGS)
AC_SUBST(LIBCAIRO_LIBS)

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
