# About

Libosmscout is a C++ library for offline map rendering, routing and location lookup
based on OpenStreetMap data.

Supported platforms:
* 32bit or 64 bit platforms in general are supported.
* Requires a compiler that supports C++11.
* Linux using recent versions of gcc or clang.
* Mac OS X and iOS using XCode/clang.
* Windows using MinGW-based gcc compiler or Visual Studio 2015.
* Android did work a while ago but is currently untested. Should work, if the
 compiler is C++11 aware.

# License

The libraries itself are under LGPL. For details see the [LICENSE](/LICENSE) file.

# Homepage

The official homepage is at: http://libosmscout.sourceforge.net/. Not that the homepage is not
updated very often and is liekly not uptodate.

# Support

Please subscribe to the mailing list at https://sourceforge.net/p/libosmscout/mailman/libosmscout-development/
and ask your questions.

# Installation

You can find detailed instruction how to get libraries and applications
build and working in the openstreetmap wiki:

http://wiki.openstreetmap.org/wiki/Libosmscout

# Documentation

You can find some documentation [here](/Documentation/)

# Automatic builds

There are automatic builds for Linux and Mac OS X can be found at
[Travis](https://travis-ci.org/Framstag/libosmscout). The Linux builds are currently based on
Ubuntu 14.04. For both operating systems clang and gcc is used as compiler.

You can find automatic builds for Windows at
[Appveyor](https://ci.appveyor.com/project/Framstag/libosmscout). Currently the build uses MSYS2/gcc.

Current build status:
* Linux, OS X: [![Build Status](https://travis-ci.org/Framstag/libosmscout.svg?branch=master)](https://travis-ci.org/Framstag/libosmscout)
* Windows: [![Build status](https://ci.appveyor.com/api/projects/status/s38jd7v5cwhwra8t?svg=true)](https://ci.appveyor.com/project/Framstag/libosmscout)
