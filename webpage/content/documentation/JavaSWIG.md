---
date: "2019-06-15T12:58:00+02:00"
title: "Java support via SWIG"
description: "Support for Java and especially Android via SWIG"
weight: 15

menu:
  main:
    Parent: "documentation"
    Weight: 15
---

In the following we describe how taccess libosmscout functionality from Java and especially Android using
[SWIG](http://www.swig.org/).

## Overall principle

Using libosmcout from Java involves multiple steps:

* Generating and building a C++ stub library containing special C API methods wrapping
  the C++ libosmscout code.
* Generating and building Java stub library containing declaration to access native C
  library functions in the C++ stub class.
* At the same time generating Java (partly proxy) classes emulating the interface
  of the corresponding C++ libosmscout classes internally using the Java/C++ stub methods.
* Loading the C++ stub library from the Java code using `System.loadLibrary()`
* Accessing the generated Java classes to implicitly use libosmscout.

## Dependencies

You need the following

* SWIG itself (Version 4.0.0 or higher)
* cmake, meson is currently not supported (but should be possible to use in principle, too)
* Java JDK (tested using Java 11, lower version should be OK, too)
* Maven

## Building libosmscout-binding to generate and build stubs

To build the various stub code call

```bash
cmake clean osmscout_binding_java
```

This generates and build the C++ stub library, it also generates the Java code.

## Build the Java stub library

cmake generates the Java code into the libosmscout-binding/Java directory in the **build**
directory (not the **source** directory). You currently have to copy the pom.xml from the
source directory into the build directory manually.

If your build directory is `<libosmscout base directory>/build` and you are in 
`<libosmscout base directory>/build/libosmscout-binding/Java`, call

```bash
cp ../../../libosmscout-binding/pom.xml .
```

After that build the Java stub library and install it in your **local** maven repository:

```bash
mvn install
```

## Build demos

Finally you can build the demos using maven. Goto `<libosmscout base directory/Java` and
call maven:

```bash
mvn package
```

You can then start the demos like regular java applications. Note that the demos
expect command line arguments similar to the C++ demos.

For the demos to work you must have the libosmscout_binding_java shared library
in the Java search path for such libraries. If you have followed the above directory
structure this would result in calling java similar to below call:

```
java -jar \
  -Djava.library.path=/home/tim/projects/OSMScout/build/libosmscout-binding \
  target/OpenDatabase-1.0-SNAPSHOT-jar-with-dependencies.jar \
  ../../maps/nordrhein-westfalen
```

## General notes

Node that due to the rather complex build process we currently do not have a regular global
CI build to make sure that everything works all the time. So expect smaller, but easy to
fix errors. Call the mailing list for help in this case.
