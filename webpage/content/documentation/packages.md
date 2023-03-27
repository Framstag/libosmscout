---
date: "2023-03-27T07:55:58+01:00"
title: "Packages"
description: "Where to find prebuild distribution packages"
weight: 4

menu:
  main:
    Parent: "documentation"
    Weight: 4
---
               
## Distribution Packages

There are currently no packages for any distributions. Managing
distribution packages is a huge effort and there is currently no
actual spoken out need for them.
        
## Binary Builds

There are currently no binary releases though this would
be possible in principle.
We think however that people that request
binary builds likely actually want packages.
                
## Source Packages

We do however create source code-based releases. There are two releases:

- A 'latest' release that gets build everytime something 
  is checked in into master. 'latest' thus represent the latest
  approved developer snapshot.
- We do make releases if there is a reasonable number of changes
  that justify other project to integrate the new version. 
  These releases follow the [CHRONVER](https://chronver.org/)
  versioning scheme.

The releases are created based on the meson build.
  
## Package Version

In the meson build the checked-in package version is always 'latest'.
The automatic builds will modify this version locally during
release building.
 
The cmake build will be adapted accordingly.

## Library version

Independently of the package version the **meson** build has a library
version that uses SEMVER. We currently do not actively manage
the semver version, but this is subject to change if required.

## Docker Images

See however documentation regarding [docker images]({{< relref "/documentation/docker.md" >}}) as a potential
way to easily get the code building on your system.

