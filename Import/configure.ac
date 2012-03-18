dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.56)
AC_INIT([Import],
        [0.1],
        [Send bugreports to <tim@teulings.org>],
        [Import])

AC_CONFIG_SRCDIR(src/Import.cpp)

AC_CONFIG_HEADERS(src/config.h)
AM_INIT_AUTOMAKE([nostdinc subdir-objects dist-bzip2 no-dist-gzip])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_LANG(C++)
AC_PROG_CXX
LT_INIT
AC_PROG_INSTALL

AX_CHECK_COMPILE_FLAG([-std=c++0x],
                      [CPPFLAGS="$CPPFLAGS -std=c++0x"])

if test "$GXX" = yes; then
  CPPFLAGS="$CPPFLAGS -Wall -pedantic -Wpointer-arith -Wsign-compare -Wundef -Wcast-qual -Wcast-align -Wwrite-strings -Wredundant-decls -Wno-long-long"
fi

PKG_CHECK_MODULES(LIBOSMSCOUT,[libosmscout])
AC_SUBST(LIBOSMSCOUT_CFLAGS)
AC_SUBST(LIBOSMSCOUT_LIBS)

PKG_CHECK_MODULES(LIBOSMSCOUTIMPORT,[libosmscout-import])
AC_SUBST(LIBOSMSCOUTIMPORT_CFLAGS)
AC_SUBST(LIBOSMSCOUTIMPORT_LIBS)

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
