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

## Preparations

Note, that if your distribution supports Qt4 and Qt5 make
sure that you install the Qt5 packages. If you have installed Qt4 and Qt5
packages in parallel you might have to install additional packages that offer
you ways to default to Qt5 or select Qt5 dynamically (e.g. qt5-default for
debian).
  
## Build

You can also find detailed build instructions
[in the OpenStreetMap wiki](http://wiki.openstreetmap.org/wiki/Libosmscout).

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

This should recursivly analyse dependencies for all project sub directories and
afterwards build them.

### Building of OSMScout2

The OSMScout2 Qt5 based demo is build using qmake if autoconf is used. The
CMake build uses it own build file.

### Running applications

To run the various tools and demos which are part of the libosmscout build
you must  make sure, that the libosmsocut libraries are found by the loader.

#### Linux/Mac OS
For this `LD_LIBRARY_PATH` has to be extended. See the `setupAutoconf.sh`
script in the top level directory for how to do it.

### Windows
Libraries are searched via `PATH` or must be in the same directory as the
executable that requires it.
