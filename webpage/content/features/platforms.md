---
date: "2016-05-29T19:40:58+02:00"
title: "Supported platforms"
description: "List of supported platforms"
weight: 1

menu:
  main:
    Parent: "features"
    Weight: 1
---

The following platforms are supported:

* Linux using gcc or clang, using autoconf tools or cmake as build system
* Mac OS X and iOS using XCode/clang
* Windows
  * Visual Studio 2013 (likely libosmscout source has to be patched slightly)
  * Visual Studio 2015
  * Visual Studio 2017
  * MinGW/[MSYS2](https://sourceforge.net/projects/msys2/)
* Android 
  * Qt
  * Initial efforts to generate Java stubs by using SWIG to alloe easy access
    to libosmscout from normal Android applications
* Sailfish OS using Qt backend
* Ubuntu phone using Qt backend

The compiler must at least support C++11.

