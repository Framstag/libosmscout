dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.56)

AC_INIT([BasemapImport],
        [0.1],
        [Send bugreports to <tim@teulings.org>],
        [BasemapImport])
AM_INIT_AUTOMAKE([nostdinc subdir-objects dist-bzip2 no-dist-gzip])

AC_CONFIG_SRCDIR(src/BasemapImport.cpp)
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS(src/config.h)

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_PROG_CXX
AC_PROG_INSTALL

LT_INIT

AC_LANG(C++)
AX_CXX_COMPILE_STDCXX_11([],[mandatory])

AX_CXXFLAGS_WARN_ALL
AS_IF([test "x$GXX" = xyes],
      [CXXFLAGS="$CXXFLAGS -Wextra -Wpointer-arith -Wundef -Wcast-qual -Wcast-align -Wredundant-decls -Wno-long-long -Wunused-variable"])

PKG_CHECK_MODULES(LIBOSMSCOUT,[libosmscout libosmscout-import])
AC_SUBST(LIBOSMSCOUT_CFLAGS)
AC_SUBST(LIBOSMSCOUT_LIBS)

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
