# About

Libosmscout is a C++ library for offline map rendering, routing and location lookup
based on OpenStreetMap data.

Supported platforms:
* 32bit or 64 bit platforms in general are supported.
* Requires a compiler that supports C++17.
* Supported build systems are CMake and Meson.
* Linux using recent versions of gcc or clang.
* Mac OS X and iOS using XCode/clang.
* Windows using MinGW-based gcc compiler or Visual Studio 2019.
* Android 7 or newer using NDK 18b or newer (with C++17 support).

# License

The libraries itself are under LGPL. For details see the [LICENSE](/LICENSE) file.

# Homepage

The official homepage is at: http://libosmscout.sourceforge.net/.

# Support

Please subscribe to the [mailing list](https://sourceforge.net/p/libosmscout/mailman/libosmscout-development/)
and ask your questions. English is the preferred language but other languages might be supported,
too.

You can also get support in the matrix room `#libosmscout.matrix.org`. While
it is a interactive chat room, an answer still may take some
time, though.


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

Automatic builds can be found at [Github Actions](https://github.com/Framstag/libosmscout/actions),
[Appveyor](https//ci.appveyor.com/project/Framstag/libosmscout) and [Wecker](https://app.wercker.com/project/byKey/39a4ba230c28d1d9e4ecae6158b283e8).
Static code analysis on [Sonar cloud](https://sonarcloud.io/dashboard?id=Framstag_libosmscout).
Goal is to check all supported platforms, compilers and build systems to keep project in good condition.

Current build status:

|Operating Systems|Provider|Status|
|-----------------|--------|------|
|iOS|Github Actions|[![Build Status](https://github.com/Framstag/libosmscout/actions/workflows/build_and%20test_on_ios.yml/badge.svg)](https://github.com/Framstag/libosmscout/actions/workflows/build_and%20test_on_ios.yml)|
|Windows, MSYS/MINGW64|Github Actions|[![Build Status](https://github.com/Framstag/libosmscout/actions/workflows/build_and%20test_on_msys.yml/badge.svg)](https://github.com/Framstag/libosmscout/actions/workflows/build_and%20test_on_msys.yml)|
|Mac OS X|Github Actions|[![Build Status](https://github.com/Framstag/libosmscout/actions/workflows/build_and%20test_on_osx.yml/badge.svg)](https://github.com/Framstag/libosmscout/actions/workflows/build_and%20test_on_osx.yml)|
|Linux (Ubuntu 20.04)|Github Actions|[![Build Status](https://github.com/Framstag/libosmscout/actions/workflows/build_and%20test_on_ubuntu_20_04.yml/badge.svg)](https://github.com/Framstag/libosmscout/actions/workflows/build_and%20test_on_ubuntu_20_04.yml)|
|Windows, Visual Studio 2019|Github Actions|[![Build Status](https://github.com/Framstag/libosmscout/actions/workflows/build_and%20test_on_vs2019.yml/badge.svg)](https://github.com/Framstag/libosmscout/actions/workflows/build_and%20test_on_vs2019.yml)|
|Android (on Ubuntu 18.04)|Github Actions|[![Build Status](https://github.com/Framstag/libosmscout/actions/workflows/build_on_ubuntu_18_04_qt_android.yml/badge.svg)](https://github.com/Framstag/libosmscout/actions/workflows/build_on_ubuntu_18_04_qt_android.yml)|
|Linux (with sanitizers)|Github Actions|[![Build Status](https://github.com/Framstag/libosmscout/actions/workflows/sanitize_on_ubuntu_20_04.yml/badge.svg)](https://github.com/Framstag/libosmscout/actions/workflows/sanitize_on_ubuntu_20_04.yml)|
