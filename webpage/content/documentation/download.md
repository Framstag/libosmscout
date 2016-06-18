---
date: "2016-05-29T19:40:58+02:00"
title: "Download source"
description: "Where to find the libosmscout sources"
weight: 1

menu:
  main:
    Parent: "documentation"
    Weight: 1
---

## Download

There are currently no regular source snapshots in form of downloadable archives
or versioned tarballs. You have to download the library sources and the demo
applications manually by cloning the git repository and build everything yourself.
The sources use autoconf or cmake to setup the build.

You can browse the reprository at https://sourceforge.net/p/libosmscout/code/ci/master/tree/.

To clone the repository use

```bash
  $ git clone git://git.code.sf.net/p/libosmscout/code libosmscout-code
```

See [Building]({{< relref "documentation/source.md" >}}) for details on how to
build the library after downloading its sources.

