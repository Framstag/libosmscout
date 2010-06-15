dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.56)
AC_INIT([libosmscout],[0.1],[tim@teulings.org],[libosmscout])

AC_CONFIG_SRCDIR(src/osmscout/Database.cpp)
AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_HEADERS([include/osmscout/private/Config.h])
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

dnl AC_MSG_CHECKING([for gcc symbol visibility support])
dnl OLDCXXFLAGS="$CXXFLAGS"
dnl CXXFLAGS="$CXXFLAGS -fvisibility=hidden -fvisibility-inlines-hidden"
dnl AC_TRY_COMPILE([],
dnl                [],
dnl                [LIB_CXXFLAGS="-fvisibility=hidden -fvisibility-inlines-hidden"
dnl                 AC_SUBST(LIB_CXXFLAGS)
dnl                 AC_MSG_RESULT(yes)
dnl                 AC_DEFINE(LIBOSMSCOUT_EXPORT_SYMBOLS,1,[libosmscout uses special gcc compiler features to export symbols])],
dnl                [AC_MSG_RESULT(no)])
dnl CXXFLAGS="$OLDCXXFLAGS"

dnl Check for libxml2
PKG_CHECK_MODULES(XML2,[libxml-2.0 >= 2.6.0],
                  [AC_SUBST(XML2_CFLAGS)
                   AC_SUBST(XML2_LIBS)])

CPPFLAGS="-DLIB_DATADIR=\\\"$datadir/$PACKAGE_NAME\\\" $CPPFLAGS"

AC_CONFIG_FILES([Makefile src/Makefile include/Makefile tests/Makefile libosmscout.pc libosmscout-uninstalled.pc])
AC_OUTPUT

