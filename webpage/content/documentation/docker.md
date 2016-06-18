---
date: "2016-05-29T19:40:58+02:00"
title: "Docker images"
description: "How to download and build libosmscout"
weight: 5

menu:
  main:
    Parent: "documentation"
    Weight: 5
---

You can find a number docker files for building libosmscout under a number
of common Linux distributions under *ci/docker/*. These docker images just
install the required packages, download the code and build it. There are
currently vaiants for building the source using autoconf and using cmake.

The images have two purposes:

* Enable the development team to quickly check, if the code builds
* Define the required packages and build steps to get the code runnung
  under a given distribution and build system.

Currently not all docker images actually build the library completely,
because of problems with packages, autoconf not detectig things..


