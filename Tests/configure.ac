dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.56)

AC_INIT(libosmscout-tests,
        [0.1],
        [Send bugreports to <tim@teulings.org>],
        [libosmscout-tests])
AM_INIT_AUTOMAKE([nostdinc subdir-objects dist-bzip2 no-dist-gzip])

AC_CONFIG_SRCDIR(src/ReaderScannerPerformance.cpp)
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

PKG_CHECK_MODULES(LIBOSMSCOUT,[libosmscout])
AC_SUBST(LIBOSMSCOUT_CFLAGS)
AC_SUBST(LIBOSMSCOUT_LIBS)

PKG_CHECK_MODULES(LIBOSMSCOUTMAP,[libosmscout-map])
AC_SUBST(LIBOSMSCOUTMAP_CFLAGS)
AC_SUBST(LIBOSMSCOUTMAP_LIBS)

PKG_CHECK_MODULES(LIBOSMSCOUTMAPQT,[libosmscout-map-qt Qt5Widgets >= 5.0.0],
                  [HASLIBOSMSCOUTMAPQT=true
                   AC_SUBST(LIBOSMSCOUTMAPQT_CFLAGS)
                   AC_SUBST(LIBOSMSCOUTMAPQT_LIBS)
                   AC_DEFINE(HAVE_LIB_OSMSCOUTMAPQT,1,[libosmscout-map-qt found])],
                  [HASLIBOSMSCOUTMAPQT=false])

AC_CHECK_PROGS(MOC, [moc-qt5 moc])

AM_CONDITIONAL(HAVE_LIB_OSMSCOUTMAPQT,[test "$HASLIBOSMSCOUTMAPQT" = "true"])

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
