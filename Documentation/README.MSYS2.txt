About
=====
This file describes the compilation of libosmscout under the MSYS2 (64bit) environment.

About MSYS2
===========

MSYS2 allows you to build and run Unix based applications under Windows. It allows
you to build libosmscout using autoconf tools and gcc under Windows.

For more information regarding MSYS2 see:
* https://msys2.github.io/ 
* https://sourceforge.net/p/msys2/wiki/MSYS2%20installation/

Required Packages
=================

We are using the MINGW64 - not the MSYS - environment, make sure to use the right shell (the MINGW
one!) for building.

Besides the base configuration as supplied by the installer and the following mandatory steps
you need the following packages for building libosmscout:
* git
* autoconf
* automake
* make
* mingw-w64-x86_64-toolchain
* mingw-w64-x86_64-libtool

For libosmscout-import you need additonally:
* mingw-w64-x86_64-protobuf
* mingw-w64-x86_64-libxml2

For the cairo backend you need additionally:
* mingw-w64-x86_64-cairo
* mingw-w64-x86_64-pango

For the Qt5 backend you need additionally:
* mingw-w64-x86_64-qt5

packages can be installed using

  pacman -S <package>

Building
========
* Source the setupAutoconf.sh in the top level directory. It sets  the pkg-config
  path to the right values for the given shell session.
  
Other recomendations
====================
Configure git to handle crlf line endings automatically:

  git config --global core.autocrlf true
