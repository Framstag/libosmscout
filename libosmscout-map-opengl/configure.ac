dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.56)

AC_INIT([libosmscout-map-opengl],
        [0.1],
        [tim@teulings.org],
        [libosmscout-map-opengl])
AM_INIT_AUTOMAKE([no-define nostdinc subdir-objects dist-bzip2])

AC_CONFIG_SRCDIR(src/osmscout/MapPainterOpenGL.cpp)
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([include/osmscout/private/Config.h include/osmscout/MapOpenGLFeatures.h])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_PROG_CXX
AC_PROG_INSTALL

LT_INIT([win32-dll disable-static])
gl_VISIBILITY
AS_IF([test "$HAVE_VISIBILITY" == "1"],
      [AC_DEFINE([OSMSCOUT_MAP_OPENGL_EXPORT_SYMBOLS],[1],[libosmscout uses special gcc compiler features to export symbols])])

AC_LANG(C++)
AX_CXX_COMPILE_STDCXX_11([],[mandatory])

AX_CXXFLAGS_WARN_ALL
AS_IF([test "x$GXX" = xyes],
      [CXXFLAGS="$CXXFLAGS -Wextra -Wpointer-arith -Wundef -Wcast-qual -Wcast-align -Wredundant-decls -Wno-long-long -Wunused-variable"])

AC_SEARCH_LIBS([sqrt],[m],[])

dnl Selfmade tests

AC_MSG_CHECKING([for WIN32])
  AC_LINK_IFELSE([
    AC_LANG_PROGRAM([[#include <windows.h>]],
                    [[GetModuleHandleA(0);]])],
    [AC_MSG_RESULT(yes);
     LDFLAGS="$LDFLAGS -no-undefined"],
    [AC_MSG_RESULT(no)])

dnl Check for libosmscout
PKG_CHECK_MODULES(LIBOSMSCOUT,[libosmscout libosmscout-map],
                  [AC_SUBST(LIBOSMSCOUT_CFLAGS)
                   AC_SUBST(LIBOSMSCOUT_LIBS)])

dnl Check for -lgl
PKG_CHECK_MODULES(LIBOPENGL,[gl],
                  [AC_SUBST(LIBOPENGL_CFLAGS)
                   AC_SUBST(LIBOPENGL_LIBS)])

AX_CHECK_GLUT
AM_CONDITIONAL(HAVE_LIB_GLUT,[test "$no_glut" != "yes"])

if test "x$ac_cv_header_GL_glut_h" = xyes; then
  AC_DEFINE([OSMSCOUT_MAP_OPENGL_HAVE_GL_GLUT_H],
             [1],
             [GLUT is defined by header GL/glut.h])
fi

if test "x$ac_cv_header_GLUT_glut_h" = xyes; then
  AC_DEFINE([OSMSCOUT_MAP_OPENGL_HAVE_GLUT_GLUT_H]
            [1],
            [GLUT is defined by header GLUT/glut.h])
fi

PKG_CHECK_MODULES(LIBFREETYPE, [freetype2],
                  HASLIBFREETYPE=true
                  [AC_SUBST(LIBFREETYPE_CFLAGS)
                   AC_SUBST(LIBFREETYPE_LIBS)
                   AC_DEFINE(HAVE_LIB_FREETYPE,1,[libfreetype found])],
                   [HASFREETYPE=false])

AM_CONDITIONAL(HAVE_LIB_FREETYPE,[test "$HASLIBFREETYPE" = "true"])

CPPFLAGS="-DLIB_DATADIR=\\\"$datadir/$PACKAGE_NAME\\\" $CPPFLAGS"

AX_CREATE_PKGCONFIG_INFO([],
                         [libosmscout libosmscout-map],
                         [-losmscoutmapopengl $LIBOPENGL_LIBS],
                         [libosmscout map rendering OpenGL backend library],
                         [$LIBOPENGL_CFLAGS],
                         [])

AC_CONFIG_FILES([Makefile src/Makefile include/Makefile])
AC_OUTPUT

