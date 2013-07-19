# ===========================================================================
#       http://www.gnu.org/software/autoconf-archive/ax_check_glut.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_CHECK_GLUT
#
# DESCRIPTION
#
#   Check for GLUT. If GLUT is found, the required compiler and linker flags
#   are included in the output variables "GLUT_CFLAGS" and "GLUT_LIBS",
#   respectively. If GLUT is not found, "no_glut" is set to "yes".
#
#   If the header "GL/glut.h" is found, "HAVE_GL_GLUT_H" is defined. If the
#   header "GLUT/glut.h" is found, HAVE_GLUT_GLUT_H is defined. These
#   preprocessor definitions may not be mutually exclusive.
#
#   You should use something like this in your headers:
#
#     # if HAVE_WINDOWS_H && defined(_WIN32)
#     #  include <windows.h>
#     # endif
#     # if defined(HAVE_GL_GLUT_H)
#     #  include <GL/glut.h>
#     # elif defined(HAVE_GLUT_GLUT_H)
#     #  include <GLUT/glut.h>
#     # else
#     #  error no glut.h
#     # endif
#
# LICENSE
#
#   Copyright (c) 2009 Braden McDaniel <braden@endoframe.com>
#   Copyright (c) 2013 Bastien Roucaries <roucaries.bastien+autoconf@gmail.com>
#
#   This program is free software; you can redistribute it and/or modify it
#   under the terms of the GNU General Public License as published by the
#   Free Software Foundation; either version 2 of the License, or (at your
#   option) any later version.
#
#   This program is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
#   Public License for more details.
#
#   You should have received a copy of the GNU General Public License along
#   with this program. If not, see <http://www.gnu.org/licenses/>.
#
#   As a special exception, the respective Autoconf Macro's copyright owner
#   gives unlimited permission to copy, distribute and modify the configure
#   scripts that are the output of Autoconf when processing the Macro. You
#   need not follow the terms of the GNU General Public License when using
#   or distributing such scripts, even though portions of the text of the
#   Macro appear in them. The GNU General Public License (GPL) does govern
#   all other use of the material that constitutes the Autoconf Macro.
#
#   This special exception to the GPL applies to versions of the Autoconf
#   Macro released by the Autoconf Archive. When you make and distribute a
#   modified version of the Autoconf Macro, you may extend this special
#   exception to the GPL to apply to your modified version as well.

#serial 13

dnl local save flags
AC_DEFUN([_AX_CHECK_GLUT_SAVE_FLAGS],
[dnl
ax_check_glut_saved_libs="${LIBS}"
ax_check_glut_saved_cflags="${CFLAGS}"
ax_check_glut_saved_cppflags="${CPPFLAGS}"
ax_check_glut_saved_ldflags="${LDFLAGS}"
])


dnl local restore flags
AC_DEFUN([_AX_CHECK_GLUT_RESTORE_FLAGS],
[dnl
LIBS="${ax_check_glut_saved_libs}"
CFLAGS="${ax_check_glut_saved_cflags}"
CPPFLAGS="${ax_check_glut_saved_cppflags}"
LDFLAGS="${ax_check_glut_saved_ldflags}"
])

dnl Default include : add windows.h
dnl see http://www.opengl.org/wiki/Platform_specifics:_Windows
dnl (acceded 20120801)
AC_DEFUN([_AX_CHECK_GLUT_INCLUDES_DEFAULT],dnl
[
  AC_INCLUDES_DEFAULT
  [
  # if defined(HAVE_WINDOWS_H) && defined(_WIN32)
  #   include <windows.h>
  # endif
  ]
])

m4_define([_AX_CHECK_GLUT_PROGRAM],
          [AC_LANG_PROGRAM([[
# if HAVE_WINDOWS_H && defined(_WIN32)
#   include <windows.h>
# endif
# ifdef HAVE_GL_GLUT_H
#   include <GL/glut.h>
# elif defined(HAVE_GLUT_GLUT_H)
#   include <GLUT/glut.h>
# else
#   error no glut.h
# endif]],
[[glutMainLoop()]])])


dnl Check headers manually (default case)
AC_DEFUN([_AX_CHECK_GLUT_HEADERS],
[AC_LANG_PUSH([C])
 _AX_CHECK_GLUT_SAVE_FLAGS()
 CFLAGS="${GLUT_CFLAGS} ${CFLAGS}"
 # see comment in _AX_CHECK_GL_INCLUDES_DEFAULT
 AC_CHECK_HEADERS([windows.h],[],[],[AC_INCLUDES_DEFAULT])
 AC_CHECK_HEADERS([GL/glut.h OpenGL/glut.h],
                         [ax_check_glut_have_headers="yes";break],
                         [ax_check_glut_have_headers_headers="no"],
			 [_AX_CHECK_GLUT_INCLUDES_DEFAULT()])
 # do not try darwin specific OpenGl/gl.h
 _AX_CHECK_GLUT_RESTORE_FLAGS()
 AC_LANG_POP([C])
])

# dnl try to found library (generic case)
# dnl $1 is set to the library to found
AC_DEFUN([_AX_CHECK_GLUT_MANUAL_LIBS_GENERIC],
[dnl
 ax_check_glut_manual_libs_generic_extra_libs="$1"
 AS_IF([test "X$ax_check_glut_manual_libs_generic_extra_libs" = "X"],
       [AC_MSG_ERROR([AX_CHECK_GLUT_MANUAL_LIBS_GENERIC argument must no be empty])])

 AC_LANG_PUSH([C])
 _AX_CHECK_GLUT_SAVE_FLAGS()
 CFLAGS="${GLUT_CFLAGS} ${CFLAGS}"
 LIBS="${GLUT_LIBS} ${LIBS}"
 AC_SEARCH_LIBS([glutMainLoop],[$ax_check_glut_manual_libs_generic_extra_libs],
                [ax_check_glut_lib_opengl="yes"],
                [ax_check_glut_lib_opengl="no"])
 AS_CASE([$ac_cv_search_glutMainLoop],
         ["none required"],[],
 	 [no],[],
 	 [GLUT_LIBS="${ac_cv_search_glutMainLoop} ${GLU_LIBS}"])
  _AX_CHECK_GLUT_RESTORE_FLAGS()
  AC_LANG_PUSH([C])
])


dnl Check library manually: subroutine must set
dnl $ax_check_glut_lib_opengl={yes,no}
dnl for windows part see
dnl   - http://www.transmissionzero.co.uk/computing/using-glut-with-mingw/
dnl   - http://user.xmission.com/~nate/glut.html
AC_DEFUN([_AX_CHECK_GLUT_MANUAL_LIBS],
[AC_REQUIRE([AC_CANONICAL_HOST])
 GLUT_LIBS="${GLUT_LIBS} ${GLU_LIBS}"
 AS_CASE([${host}],
         # try first cygwin version
         [*-cygwin*],[_AX_CHECK_GLUT_MANUAL_LIBS_GENERIC([GLUT glut MesaGLUT freeglut freeglut32 glut32])],
         # try first native
	 [*-mingw*],[_AX_CHECK_GLUT_MANUAL_LIBS_GENERIC([glut32 GLUT glut MesaGLUT freeglut freeglut32])],
	 [_AX_CHECK_GLUT_MANUAL_LIBS_GENERIC([GLUT glut freeglut MesaGLUT])])

 AC_CACHE_CHECK([for OpenGL Utility Toolkit (GLUT) libraries],[ax_cv_check_glut_lib_opengl],
               	[ax_cv_check_glut_lib_opengl="${ax_check_glut_lib_opengl}"])
 ax_check_glut_lib_opengl="${ax_cv_check_glut_lib_opengl}"
])

# compile the example program
AC_DEFUN([_AX_CHECK_GLUT_COMPILE],
[dnl
 AC_LANG_PUSH([C])
 _AX_CHECK_GLUT_SAVE_FLAGS()
 CFLAGS="${GLUT_CFLAGS} ${CFLAGS}"
 AC_COMPILE_IFELSE([_AX_CHECK_GLUT_PROGRAM],
                   [ax_check_glut_compile_opengl="yes"],
                   [ax_check_glut_compile_opengl="no"])
 _AX_CHECK_GLUT_RESTORE_FLAGS()
 AC_LANG_POP([C])
])

# compile the example program (cache)
AC_DEFUN([_AX_CHECK_GLUT_COMPILE_CV],
[dnl
 AC_CACHE_CHECK([for compiling a minimal OpenGL Utility Toolkit (GLUT) program],[ax_cv_check_glut_compile_opengl],
                [_AX_CHECK_GLUT_COMPILE()
                 ax_cv_check_glut_compile_opengl="${ax_check_glut_compile_opengl}"])
 ax_check_glut_compile_opengl="${ax_cv_check_glut_compile_opengl}"
])

# link the example program
AC_DEFUN([_AX_CHECK_GLUT_LINK],
[dnl
 AC_LANG_PUSH([C])
 _AX_CHECK_GLUT_SAVE_FLAGS()
 CFLAGS="${GLUT_CFLAGS} ${CFLAGS}"
 LIBS="${GLUT_LIBS} ${LIBS}"
 LDFLAGS="${GLUT_LDFLAGS} ${LDFLAGS}"
 AC_LINK_IFELSE([_AX_CHECK_GLUT_PROGRAM],
                [ax_check_glut_link_opengl="yes"],
                [ax_check_glut_link_opengl="no"])
 _AX_CHECK_GLUT_RESTORE_FLAGS()
 AC_LANG_POP([C])
])

# link the example program (cache)
AC_DEFUN([_AX_CHECK_GLUT_LINK_CV],
[dnl
 AC_CACHE_CHECK([for linking a minimal OpenGL Utility Toolkit (GLUT) program],[ax_cv_check_glut_link_opengl],
                [_AX_CHECK_GLUT_LINK()
                 ax_cv_check_glut_link_opengl="${ax_check_glut_link_opengl}"])
 ax_check_glut_link_opengl="${ax_cv_check_glut_link_opengl}"
])


# manually check GLUT
AC_DEFUN([_AX_CHECK_GLUT_MANUAL],dnl
[
GLUT_CFLAGS="${GLUT_CFLAGS} ${GLU_CFLAGS}"
_AX_CHECK_GLUT_HEADERS

AS_IF([test "X$ax_check_glut_have_headers" = "Xyes"],
      [_AX_CHECK_GLUT_MANUAL_LIBS],
      [ax_check_glut_lib="no"])

AS_IF([test "X$ax_check_glut_lib_opengl" = "Xyes"],
      [_AX_CHECK_GLUT_COMPILE_CV()],
      [ax_cv_check_glut_compile_opengl="no"])

AS_IF([test "X$ax_cv_check_glut_compile_opengl" = "Xyes"],
      [_AX_CHECK_GLUT_LINK_CV()],
      [ax_cv_check_glut_link_opengl="no"])

AS_IF([test "X$ax_cv_check_glut_link_opengl" = "Xyes"],
       [no_glut="no"],
       [no_glut="yes"])
])


# main entry point
AC_DEFUN([AX_CHECK_GLUT],
[dnl
 AC_REQUIRE([AX_CHECK_GL])dnl
 AC_REQUIRE([AX_CHECK_GLU])dnl

 _AX_CHECK_GLUT_MANUAL

 AC_MSG_CHECKING([for a working OpenGL Utility Toolkit (GLUT) implementation])
 AS_IF([test "X$no_glut" = "Xno"],
       [AC_MSG_RESULT([yes])
        AC_MSG_CHECKING([for CFLAGS needed for OpenGL Utility Toolkit (GLUT)])
        AC_MSG_RESULT(["${GLUT_CFLAGS}"])
        AC_MSG_CHECKING([for LIBS needed for OpenGL Utility Toolkit (GLUT)])
        AC_MSG_RESULT(["${GLUT_LIBS}"])
        AC_MSG_CHECKING([for LDFLAGS needed for OpenGL Utility Toolkit (GLUT)])
        AC_MSG_RESULT(["${GLUT_LDFLAGS}"])],
       [AC_MSG_RESULT([no])
        GLUT_CFLAGS=""
        GLUT_LIBS=""
        GLUT_LDFLAGS=""])

 AC_SUBST([GLUT_CFLAGS])
 AC_SUBST([GLUT_LIBS])
 AC_SUBST([GLUT_LDFLAGS])
])
