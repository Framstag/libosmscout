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

The official homepage is at: http://libosmscout.sourceforge.net/.

# Support

Please subscribe to the [mailing list](https://sourceforge.net/p/libosmscout/mailman/libosmscout-development/)
and ask your questions. English is the preferred language but other languages might be supported,
too.

# Installation

You can find detailed instruction how to get libraries and applications
build and working and other introductory documentation on the
[homepage](http://libosmscout.sourceforge.net/documentation/).

The documentation in the [OpenStreepMap Wiki](http://wiki.openstreetmap.org/wiki/Libosmscout)
is currently still correct but is not activily maintained by the
libosmscout team.

# Features

You can find a list of features [here](http://libosmscout.sourceforge.net/features/).
Note that the features pages are currently not up to date. We are unsure how to
best represent all the features of libosmscout.

# Documentation

You can find some documentation and tutorials on the [homepage](http://libosmscout.sourceforge.net)
and some other documentation in the [git repository](/Documentation/).

There are a number of demo applications that show how to make use of the various
features of the library.

We plan to move all documentation for the repository to the homepage.

# Automatic builds

Automatic builds for Linux and Mac OS X can be found at
[Travis](https://travis-ci.org/Framstag/libosmscout). The Linux builds are currently based on
Ubuntu 14.04. For both operating systems clang and gcc is used as compiler.

You can find automatic builds for Windows at
[Appveyor](https://ci.appveyor.com/project/Framstag/libosmscout). There are builds
for using MinGW (autoconf, cmake) and VisualStudio (cmake).

Current build status:

|Operating Systems|Provider|Status|
|-----------------|--------|------|
|Linux, Mac OS|Travis-CI| [![Build Status](https://travis-ci.org/Framstag/libosmscout.svg?branch=master)](https://travis-ci.org/Framstag/libosmscout)|
|Windows|Appveyor|[![Build status](https://ci.appveyor.com/api/projects/status/s38jd7v5cwhwra8t?svg=true)](https//ci.appveyor.com/project/Framstag/libosmscout)|
|Linux|Circle CI|[![Build Status](https://circleci.com/gh/Framstag/libosmscout.svg?style=shield&circle-token=:circle-token)](https://circleci.com/gh/Framstag/libosmscout)|
