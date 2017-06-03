dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.56)

AC_INIT([libosmscout-map-iOSX],[0.1],[tim@teulings.org],[libosmscout-map-iOSX])
AM_INIT_AUTOMAKE([no-define nostdinc subdir-objects dist-bzip2])

AC_CONFIG_SRCDIR(src/osmscout/MapPainterIOS.mm)
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([include/osmscout/private/Config.h])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_PROG_OBJCXX
AC_PROG_INSTALL

LT_INIT([win32-dll disable-static])
gl_VISIBILITY
AS_IF([test "$HAVE_VISIBILITY" == "1"],
      [AC_DEFINE([OSMSCOUT_MAP_IOSX_EXPORT_SYMBOLS],[1],[libosmscout uses special gcc compiler features to export symbols])])

AC_LANG(Objective C++)

AC_SEARCH_LIBS([sqrt],[m],[])

dnl Check for libosmscout
PKG_CHECK_MODULES(LIBOSMSCOUT,[libosmscout],
                  [AC_SUBST(LIBOSMSCOUT_CFLAGS)
                   AC_SUBST(LIBOSMSCOUT_LIBS)])

dnl Check for libosmscout-map
PKG_CHECK_MODULES(LIBOSMSCOUTMAP,[libosmscout-map],
                  [AC_SUBST(LIBOSMSCOUTMAP_CFLAGS)
                   AC_SUBST(LIBOSMSCOUTMAP_LIBS)])

AC_MSG_CHECKING([Checking for OS X/iOS])
AS_CASE([$host],
        [*-apple-darwin*],[AC_MSG_RESULT([yes])],
        [AC_MSG_RESULT([no])
         AC_MSG_ERROR([This backend oly builds under OS X/iOS, not under $host])])

CPPFLAGS="-DLIB_DATADIR=\\\"$datadir/$PACKAGE_NAME\\\" $CPPFLAGS"

AX_CREATE_PKGCONFIG_INFO([],
                         [libosmscout libosmscout-map],
                         [-losmscoutmapiOSX $LIBOSMSCOUTMAP_LIBS],
                         [libosmscout map rendering iOS/Mac OS X backend library],
                         [],
                         [])

AC_CONFIG_FILES([Makefile src/Makefile include/Makefile])
AC_OUTPUT
