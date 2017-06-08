---
date: "2016-05-29T19:40:58+02:00"
title: "Building"
description: "How to build libosmscout youself"
weight: 3

menu:
  main:
    Parent: "documentation"
    Weight: 3
---

## Dependencies

Libosmscout has the following optional and required dependencies:

libxml2 (optional)
: libxml2 is required for parsing *.osm file during import

protobuf-c, protobuf-compiler (optional)
: These are required for parsing *.pbf files

marisa (optional)
: for an additional location full text search index

libagg (optional),
freetype (optional)
: for the agg backend. If you use agg, freetype must be available, too

cairo (optional),
pango (optional)
: for the cairo backend, if pango is available it will be used for complex
  text rendering instead of the cairo toy font rendering code

Qt5 (optional)
: for the Qt5 backend. 

freeglut (optional),
glu (optional)
: for the OpenGL demo

Currently the library does compile without any external dependencies, however
since you need to be able to import OSM data to make any use of the library
you should at least have libxml2 or protobuf available.

## Build systems supported

Libosmscout supports autoconf based builds for Unix and Unix-like systems. It also
support cmake. 

## Preparations

Note, that if your distribution supports Qt4 and Qt5 make
sure that you install the Qt5 packages. If you have installed Qt4 and Qt5
packages in parallel you might have to install additional packages that offer
you ways to default to Qt5 or select Qt5 dynamically (e.g. qt5-default for
debian).

### Setup under Linux

The concrete packages you have to install depends on the distribution you use.
Libosmscout provides a number of Docker images under `ci/docker`. take the look at
the corresponding docker file for your distribution and the used `install.sh`and
`build.sh`scripts to se, how you can setup a valid environment.

### Setup for VisualStudio

Note that there is also a central Appveyor build, that uses a similar setup
as described here.

Please use the cmake based build for VisualStudio project setup. You just need
to import the `CMakeLists.txt` into VisualStudio. There is currently no automatic
setup or installation guide for installing dependencies.

### Setup for Mac OS X

Note that there is also a central Travis-CI build, that uses a similar setup
as described here.

To build libosmscout under Mac OS X you should have installed:

* Xcode
* [homebrew](http://docs.brew.sh/)

Using homebrew you install the following packages:

* cmake
* pkg-config
* gettext
* prptobuf
* libxml2
* libagg
* Qt5
* pango
* cairo

Follow the hint during package installation regarding visibility of the
installed packages. You may have to extend search paths and similar.

At the time the article has been written this are for example:

```bash
brew link --force gettext
brew link --force libxml2
brew link --force qt5
```

For cmake based build you also have to install:

* cmake

You can then start with a normal cmake build.

For an autoconf based build:

* autoconf
* automake
* libtool

This installs all packages required for an autoconf based
build.

The OSX/iOS backend and demo is currently not build this way. Above installations
are enough to use the Qt5 based OSMScout2 demo application though.

It looks like the qmake based build does not handle rpath for shared
libraries correctly so OSMScout2 does not find the referenced locally
build libosmscout binaries. The cmake based does not have this problem.

### Setup for MinGW/MSYS

Note that there is also a central Appveyor build, that uses a similar setup
as described here. See the Appveyor configuration for setup details.


## Build

You can also find detailed build (but old and potentially not correct anymore)
instructions [in the OpenStreetMap wiki](http://wiki.openstreetmap.org/wiki/Libosmscout).

### autoconf based build

In the top level directory type:

```bash
  $ . setupAutoconf.sh
  $ make full
```

The `setupAutoconf.sh script` will extend `PKG_CONFIG_PATH` and
`LD_LIBRARY_PATH` so that the individual library projects are found during
the `configure` step and that the libraries are found by the loader
after they have been build.

The top level Makefile should first generate the configure scripts for all
project subdirectories, then it calls the configure scripts for all
sub directories and finally (if a Makefile was generated during the
configure call) call make for all project sub directories.

### cmake based build

In the top level directory type:

```bash
  $ mkdir build
  $ cd build
  $ cmake ..
  $ make
```

This should recursively analyse dependencies for all project sub directories and
afterwards build them.

Real life example for building using VisualStudio:
```bash
mkdir build
cd build
cmake .. -G "Visual Studio 14 2015" -DCMAKE_SYSTEM_VERSION=10.0.10586.0 \
  -DCMAKE_INSTALL_PREFIX=D:\Mine\OpenSource\osmlib
cmake --build . --config Release --target install
```

Real life example for building using Xcode:
```bash
mkdir build
cd build
cmake -G "Xcode" ..
```

You can then import the Xcode project created in the build directory.

## Building of OSMScout2

The OSMScout2 Qt5 based demo is build using qmake if autoconf is used. The
CMake build uses it own build file.

## Running applications

To run the various tools and demos which are part of the libosmscout build
you must  make sure, that the libosmscout libraries are found by the loader.

#### Linux/Mac OS
For this `LD_LIBRARY_PATH` has to be extended. See the `setupAutoconf.sh`
script in the top level directory for how to do it.

Note that current Mac OS X versions doe not support additional
library search paths via environment variables anymore. Location of the
library hass to be set during compile. cmake and autoconf (and libtool) based
do this correctly, the qmake build for OSMScout2 and Styleconfig currently
does not.

#### Windows
Libraries are searched via `PATH` or must be in the same directory as the
executable that requires it.

