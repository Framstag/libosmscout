dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.56)
AC_INIT([libosmscout-map],[0.1],[tim@teulings.org],[libosmscout-map])

AC_CONFIG_SRCDIR(src/osmscout/MapPainter.cpp)
AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_HEADERS([include/osmscout/Private/Config.h include/osmscout/MapFeatures.h])
AM_INIT_AUTOMAKE([no-define nostdinc subdir-objects dist-bzip2])
AC_PROG_CXX
AC_LANG(C++)
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL
AC_PROG_INSTALL

AC_TYPE_SIZE_T

AC_CHECK_FUNCS([mmap])

AC_SEARCH_LIBS([sqrt],[m])

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

dnl Check for libosmscout
PKG_CHECK_MODULES(LIBOSMSCOUT,[libosmscout],
                  [AC_SUBST(LIBOSMSCOUT_CFLAGS)
                   AC_SUBST(LIBOSMSCOUT_LIBS)])

PKG_CHECK_MODULES(LIBCAIRO, cairo,
                  HASCAIRO=true
                  [AC_SUBST(LIBCAIRO_CFLAGS)
                   AC_SUBST(LIBCAIRO_LIBS)
                   AC_DEFINE(HAVE_LIB_CAIRO,1,[libcairo found])],
                   [])
                   
AM_CONDITIONAL(HAVE_LIB_CAIRO,[test -n "$HASCAIRO"])

PKG_CHECK_MODULES(LIBQTGUI, QtGui > 4.6.0,
                  HASQTGUI=true
                  [AC_SUBST(LIBQTGUI_CFLAGS)
                   AC_SUBST(LIBQTGUI_LIBS)
                   AC_DEFINE(HAVE_LIB_QTGUI,1,[libQtGui found])],
                   [])
                   
AM_CONDITIONAL(HAVE_LIB_QTGUI,[test -n "$HASQTGUI"])

PKG_CHECK_MODULES(LIBPNG,libpng)
AC_SUBST(LIBPNG_CFLAGS)
AC_SUBST(LIBPNG_LIBS)

CPPFLAGS="-DLIB_DATADIR=\\\"$datadir/$PACKAGE_NAME\\\" $CPPFLAGS"

AC_CONFIG_FILES([Makefile src/Makefile include/Makefile libosmscout-map.pc libosmscout-map-uninstalled.pc])
AC_OUTPUT

