dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.56)

AC_INIT([libosmscout-binding],
         [0.1],
         [tim@teulings.org],
         [libosmscout])
AM_INIT_AUTOMAKE([no-define nostdinc subdir-objects dist-bzip2])

AC_CONFIG_SRCDIR(src/swig/osmscout/Database.i)
AC_CONFIG_MACRO_DIR([m4])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_PROG_CXX
AC_PROG_INSTALL
LT_INIT([win32-dll disable-static])

AC_LANG([C++])
AX_CXX_COMPILE_STDCXX([11],[noext],[mandatory])

AX_PKG_SWIG(3.0.8, [], [ AC_MSG_ERROR([SWIG is required to build]) ])
AX_SWIG_ENABLE_CXX

AX_JNI_INCLUDE_DIR

for JNI_INCLUDE_DIR in $JNI_INCLUDE_DIRS
do
    JNI_CPPFLAGS="$JNI_CPPFLAGS -I$JNI_INCLUDE_DIR"
done
AC_SUBST(JNI_CPPFLAGS, $JNI_CPPFLAGS)
    
PKG_CHECK_MODULES(LIBOSMSCOUT,[libosmscout])
AC_SUBST(LIBOSMSCOUT_CFLAGS)
AC_SUBST(LIBOSMSCOUT_LIBS)

AC_CONFIG_FILES([Makefile src/Makefile src/swig/Makefile src/java/Makefile])
AC_OUTPUT

