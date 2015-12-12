# ===========================================================================
#       http://www.gnu.org/software/autoconf-archive/ax_check_glu.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_CHECK_GLU
#
# DESCRIPTION
#
#   Check for GLU. If GLU is found, the required preprocessor and linker
#   flags are included in the output variables "GLU_CFLAGS" and "GLU_LIBS",
#   respectively. If no GLU implementation is found, "no_glu" is set to
#   "yes".
#
#   If the header "GL/glu.h" is found, "HAVE_GL_GLU_H" is defined. If the
#   header "OpenGL/glu.h" is found, HAVE_OPENGL_GLU_H is defined. These
#   preprocessor definitions may not be mutually exclusive.
#
#   You should use something like this in your headers:
#
#     # if defined(HAVE_WINDOWS_H) && defined(_WIN32)
#     #  include <windows.h>
#     # endif
#     # if defined(HAVE_GL_GLU_H)
#     #  include <GL/glu.h>
#     # elif defined(HAVE_OPENGL_GLU_H)
#     #  include <OpenGL/glu.h>
#     # else
#     #  error no glu.h
#     # endif
#
#   Some implementations (in particular, some versions of Mac OS X) are
#   known to treat the GLU tesselator callback function type as "GLvoid
#   (*)(...)" rather than the standard "GLvoid (*)()". If the former
#   condition is detected, this macro defines "HAVE_VARARGS_GLU_TESSCB".
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

#serial 18

# exemple program
m4_define([_AX_CHECK_GLU_PROGRAM],
          [AC_LANG_PROGRAM([[
# if defined(HAVE_WINDOWS_H) && defined(_WIN32)
#   include <windows.h>
# endif
# ifdef HAVE_GL_GLU_H
#   include <GL/glu.h>
# elif defined(HAVE_OPENGL_GLU_H)
#   include <OpenGL/glu.h>
# else
#   error no glu.h
# endif
]],[[gluBeginCurve(0)]])])


dnl Default include : add windows.h
dnl see http://www.opengl.org/wiki/Platform_specifics:_Windows
dnl (acceded 20120801)
AC_DEFUN([_AX_CHECK_GLU_INCLUDES_DEFAULT],dnl
[
  AC_INCLUDES_DEFAULT
  [
  # if defined(HAVE_WINDOWS_H) && defined(_WIN32)
  #   include <windows.h>
  # endif
  ]
])

dnl local save flags
AC_DEFUN([_AX_CHECK_GLU_SAVE_FLAGS],
[dnl
ax_check_glu_saved_libs="${LIBS}"
ax_check_glu_saved_cflags="${CFLAGS}"
ax_check_glu_saved_cppflags="${CPPFLAGS}"
ax_check_glu_saved_ldflags="${LDFLAGS}"
])


dnl local restore flags
AC_DEFUN([_AX_CHECK_GLU_RESTORE_FLAGS],
[dnl
LIBS="${ax_check_glu_saved_libs}"
CFLAGS="${ax_check_glu_saved_cflags}"
CPPFLAGS="${ax_check_glu_saved_cppflags}"
LDFLAGS="${ax_check_glu_saved_ldflags}"
])


# compile the example program
AC_DEFUN([_AX_CHECK_GLU_COMPILE],
[dnl
 AC_LANG_PUSH([C])
 _AX_CHECK_GLU_SAVE_FLAGS()
 CFLAGS="${GLU_CFLAGS} ${CFLAGS}"
 AC_COMPILE_IFELSE([_AX_CHECK_GLU_PROGRAM],
                   [ax_check_glu_compile_opengl="yes"],
                   [ax_check_glu_compile_opengl="no"])
 _AX_CHECK_GLU_RESTORE_FLAGS()
 AC_LANG_POP([C])
])

# compile the example program (cache)
AC_DEFUN([_AX_CHECK_GLU_COMPILE_CV],
[dnl
 AC_CACHE_CHECK([for compiling a minimal OpenGL Utility (GLU) program],[ax_cv_check_glu_compile_opengl],
                [_AX_CHECK_GLU_COMPILE()
                 ax_cv_check_glu_compile_opengl="${ax_check_glu_compile_opengl}"])
 ax_check_glu_compile_opengl="${ax_cv_check_glu_compile_opengl}"
])

# link the example program
AC_DEFUN([_AX_CHECK_GLU_LINK],
[dnl
 AC_LANG_PUSH([C])
 _AX_CHECK_GLU_SAVE_FLAGS()
 CFLAGS="${GLU_CFLAGS} ${CFLAGS}"
 LIBS="${GLU_LIBS} ${LIBS}"
 LDFLAGS="${GLU_LDFLAGS} ${LDFLAGS}"
 AC_LINK_IFELSE([_AX_CHECK_GLU_PROGRAM],
                [ax_check_glu_link_opengl="yes"],
                [ax_check_glu_link_opengl="no"])
 _AX_CHECK_GLU_RESTORE_FLAGS()
 AC_LANG_POP([C])
])

# link the example program (cache)
AC_DEFUN([_AX_CHECK_GLU_LINK_CV],
[dnl
 AC_CACHE_CHECK([for linking a minimal OpenGL Utility (GLU) program],[ax_cv_check_glu_link_opengl],
                [_AX_CHECK_GLU_LINK()
                 ax_cv_check_glu_link_opengl="${ax_check_glu_link_opengl}"])
 ax_check_glu_link_opengl="${ax_cv_check_glu_link_opengl}"
])

dnl Check headers manually (default case)
AC_DEFUN([_AX_CHECK_GLU_HEADERS],
[AC_LANG_PUSH([C])
 _AX_CHECK_GLU_SAVE_FLAGS()
 CFLAGS="${GLU_CFLAGS} ${CFLAGS}"
 # see comment in _AX_CHECK_GL_INCLUDES_DEFAULT
 AC_CHECK_HEADERS([windows.h],[],[],[AC_INCLUDES_DEFAULT])
 AC_CHECK_HEADERS([GL/glu.h OpenGL/glu.h],
                         [ax_check_glu_have_headers="yes";break],
                         [ax_check_glu_have_headers_headers="no"],
			 [_AX_CHECK_GLU_INCLUDES_DEFAULT()])
 # do not try darwin specific OpenGl/gl.h
 _AX_CHECK_GLU_RESTORE_FLAGS()
 AC_LANG_POP([C])
])

# check tesselation callback function signature.
m4_define([_AX_CHECK_GLU_VARARGS_TESSVB_PROGRAM],
[AC_LANG_PROGRAM([[
# if defined(HAVE_WINDOWS_H) && defined(_WIN32)
#   include <windows.h>
# endif
# ifdef HAVE_GL_GLU_H
#   include <GL/glu.h>
# elif defined(HAVE_OPENGL_GLU_H)
#   include <OpenGL/glu.h>
# else
#   error no glu.h
# endif
]],
[[GLvoid (*func)(...); gluTessCallback(0, 0, func)]])
])

# compile the tesselation callback function program
# test with c++
AC_DEFUN([_AX_CHECK_GLU_COMPILE_VARARGS_TESSVB_PROGRAM],
[AC_REQUIRE([AC_PROG_CXX])dnl

 AC_LANG_PUSH([C++])
 _AX_CHECK_GLU_SAVE_FLAGS()
 CFLAGS="${GLU_CFLAGS} ${CFLAGS}"
 AC_COMPILE_IFELSE([_AX_CHECK_GLU_VARARGS_TESSVB_PROGRAM],
                   [ax_check_glu_compile_varargs_tessvb_program="yes"],
                   [ax_check_glu_compile_varargs_tessvb_program="no"])
 _AX_CHECK_GLU_RESTORE_FLAGS()
 AC_LANG_POP([C++])
])


#
# Some versions of Mac OS X include a broken interpretation of the GLU
# tesselation callback function signature.
#
AC_DEFUN([_AX_CHECK_GLU_VARARGS_TESSVB],
[
AC_CACHE_CHECK([for varargs OpenGL Utility (GLU) tesselator callback function type],
                [ax_cv_varargs_glu_tesscb],
		[_AX_CHECK_GLU_COMPILE_VARARGS_TESSVB_PROGRAM
		 ax_cv_varargs_glu_tesscb="${ax_check_glu_compile_varargs_tessvb_program}"])
ax_check_glu_compile_varargs_tessvb_program="${ax_cv_varargs_glu_tesscb}"

AS_IF([test X$ax_cv_varargs_glu_tesscb = Xyes],
      [AC_DEFINE([HAVE_VARARGS_GLU_TESSCB], [1],
                 [Use nonstandard varargs form for the GLU tesselator callback])])
])


# dnl try to found library (generic case)
# dnl $1 is set to the library to found
AC_DEFUN([_AX_CHECK_GLU_MANUAL_LIBS_GENERIC],
[dnl
 ax_check_glu_manual_libs_generic_extra_libs="$1"
 AS_IF([test "X$ax_check_glu_manual_libs_generic_extra_libs" = "X"],
       [AC_MSG_ERROR([AX_CHECK_GLU_MANUAL_LIBS_GENERIC argument must no be empty])])

 AC_LANG_PUSH([C])
 _AX_CHECK_GLU_SAVE_FLAGS()
 CFLAGS="${GLU_CFLAGS} ${CFLAGS}"
 LIBS="${GLU_LIBS} ${LIBS}"
 AC_SEARCH_LIBS([gluBeginCurve],[$ax_check_glu_manual_libs_generic_extra_libs],
                [ax_check_glu_lib_opengl="yes"],
                [ax_check_glu_lib_opengl="no"])
 AS_CASE([$ac_cv_search_gluBeginCurve],
         ["none required"],[],
 	 [no],[],
 	 [GLU_LIBS="${ac_cv_search_gluBeginCurve} ${GLU_LIBS}"])
  _AX_CHECK_GLU_RESTORE_FLAGS()
  AC_LANG_PUSH([C])
])


dnl Check library manually: subroutine must set
dnl $ax_check_gl_lib_opengl={yes,no}
AC_DEFUN([_AX_CHECK_GLU_MANUAL_LIBS],
[AC_REQUIRE([AC_CANONICAL_HOST])
 GLU_LIBS="${GLU_LIBS} ${GL_LIBS}"
 AS_CASE([${host}],
         # try first cygwin version
         [*-cygwin*],[_AX_CHECK_GLU_MANUAL_LIBS_GENERIC([GLU glu MesaGLU glu32])],
         # try first native
	 [*-mingw*],[_AX_CHECK_GLU_MANUAL_LIBS_GENERIC([glu32 GLU glu MesaGLU])],
	 [_AX_CHECK_GLU_MANUAL_LIBS_GENERIC([GLU glu MesaGLU])])

 AC_CACHE_CHECK([for OpenGL Utility (GLU) libraries],[ax_cv_check_glu_lib_opengl],
               	[ax_cv_check_glu_lib_opengl="${ax_check_glu_lib_opengl}"])
 ax_check_glu_lib_opengl="${ax_cv_check_glu_lib_opengl}"
])


dnl Manual way to detect GLU
AC_DEFUN([_AX_CHECK_GLU_MANUAL],
[dnl

# inherit cflags
GLU_CFLAGS="${GLU_CFLAGS} ${GL_CFLAGS}"

# check headers
_AX_CHECK_GLU_HEADERS

AS_IF([test "X$ax_check_glu_have_headers" = "Xyes"],
      [_AX_CHECK_GLU_MANUAL_LIBS],
      [ax_check_glu_lib_opengl="no"])

AS_IF([test "X$ax_check_glu_lib_opengl" = "Xyes"],
      [_AX_CHECK_GLU_COMPILE_CV()],
      [ax_cv_check_glu_compile_opengl="no"])

AS_IF([test "X$ax_cv_check_glu_compile_opengl" = "Xyes"],
      [_AX_CHECK_GLU_LINK_CV()],
      [ax_cv_check_glu_link_opengl="no"])

AS_IF([test "X$ax_cv_check_glu_link_opengl" = "Xyes"],
      [no_glu="no"],
      [no_glu="yes"])
])

# detect using pkgconfig
AC_DEFUN([_AX_CHECK_GLU_PKG_CONFIG],
[
 AC_REQUIRE([PKG_PROG_PKG_CONFIG])

 PKG_CHECK_MODULES([GLU],[glu],[ax_check_glu_pkg_config=yes],[ax_check_glu_pkg_config=no])

 AS_IF([test "X$ax_check_glu_pkg_config" = "Xyes"],[
        # check headers
        AC_LANG_PUSH([C])
 	_AX_CHECK_GLU_SAVE_FLAGS()
        CFLAGS="${GLU_CFLAGS} ${CFLAGS}"
        AC_CHECK_HEADERS([windows.h],[],[],[AC_INCLUDES_DEFAULT])
        AC_CHECK_HEADERS([GL/glu.h OpenGL/glu.h],
                         [ax_check_glu_have_headers="yes";break],
                         [ax_check_glu_have_headers_headers="no"],
			 [_AX_CHECK_GLU_INCLUDES_DEFAULT()])
        _AX_CHECK_GLU_RESTORE_FLAGS()
	AC_LANG_POP([C])
	AC_CACHE_CHECK([for OpenGL Utility (GLU) headers],[ax_cv_check_glu_have_headers],
               	       [ax_cv_check_glu_have_headers="${ax_check_glu_have_headers}"])

        # pkgconfig library are suposed to work ...
        AS_IF([test "X$ax_cv_check_glu_have_headers" = "Xno"],
              [AC_MSG_ERROR("Pkgconfig detected OpenGL Utility (GLU) library has no headers!")])

	_AX_CHECK_GLU_COMPILE_CV()
	AS_IF([test "X$ax_cv_check_glu_compile_opengl" = "Xno"],
              [AC_MSG_ERROR("Pkgconfig detected OpenGL Utility (GLU) library could not be used for compiling minimal program!")])

	_AX_CHECK_GLU_LINK_CV()
	AS_IF([test "X$ax_cv_check_glu_link_opengl" = "Xno"],
              [AC_MSG_ERROR("Pkgconfig detected OpenGL Utility (GLU) library could not be used for linking minimal program!")])
  ])
])

# entry point
AC_DEFUN([AX_CHECK_GLU],dnl
[
 AC_REQUIRE([AX_CHECK_GL])
 AC_REQUIRE([PKG_PROG_PKG_CONFIG])

 # set flags
 no_glu="yes"
 have_GLU="no"

 AC_MSG_CHECKING([for a working OpenGL Utility (GLU) implementation by pkg-config])
 # try first pkgconfig
 AS_IF([test "X${PKG_CONFIG}" = "X"],
       [AC_MSG_RESULT([no])
        ax_check_glu_pkg_config=no],
       [AC_MSG_RESULT([yes])
        _AX_CHECK_GLU_PKG_CONFIG()])

 # if no pkg-config or pkg-config fail try manual way
 AS_IF([test "X$ax_check_glu_pkg_config" = "Xno"],
       [_AX_CHECK_GLU_MANUAL()],
       [no_glu=no])

 # check broken implementation
 AS_IF([test "X$no_glu" = "Xno"],
       [_AX_CHECK_GLU_VARARGS_TESSVB],[])

 AC_MSG_CHECKING([for a working OpenGL Utility (GLU) implementation])
 AS_IF([test "X$no_glu" = "Xno"],
       [have_GLU="yes"
        AC_MSG_RESULT([yes])
        AC_MSG_CHECKING([for CFLAGS needed for OpenGL Utility (GLU)])
        AC_MSG_RESULT(["${GLU_CFLAGS}"])
        AC_MSG_CHECKING([for LIBS needed for OpenGL Utility (GLU)])
        AC_MSG_RESULT(["${GLU_LIBS}"])
        AC_MSG_CHECKING([for LDFLAGS needed for OpenGL Utility (GLU)])
        AC_MSG_RESULT(["${GLU_LDFLAGS}"])],
       [AC_MSG_RESULT([no])
        GLU_CFLAGS=""
        GLU_LIBS=""
        GLU_LDFLAGS=""])

 AC_SUBST([GLU_CFLAGS])
 AC_SUBST([GLU_LIBS])
 AC_SUBST([GLU_LDFLAGS])

])
