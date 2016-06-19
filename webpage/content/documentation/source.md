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

libagg (optional)
freetype (optional)
: for the agg backend. If you use agg, freetype must be available, too

cairo (optional)
pango (optional)
: for the cairo backend, if pango is available it will be used for complex
  text rendering instead of the cairo toy font rendering code

Qt (optional)
: for the Qt backend

freeglut (optional)
glu (optional)
: for the OpenGL demo

Currently the library does compile without any external dependencies, however
since you need to be able to import OSM data to make any use of the library
you should at least have libxml2 or protobuf available.

## Build

You can also find detailed build instructions
[in the OpenStreetMap wiki](http://wiki.openstreetmap.org/wiki/Libosmscout).

### autoconf based build

In the top level directory type:

```bash
  $ make full
```

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

The OSMScout2 Qt based demo is build using qmake if autoconf is used. The
CMake build uses it own build file.

