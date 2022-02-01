# Mac OSX and iOS sample projet

This projet is a sample Objective-C application using the OSMScout libraries to display a interactive map from imported and compiled cartographic data from the OpenStreetMap project using OSMScout tools.

This project target both Mac OS and iOS devices and simulators. The project include a demo map compiled from the OpenStreetMap data of the [Greater London](https://download.geofabrik.de/europe/great-britain/england/greater-london.html) area provided by GeoFabrik. 

To compile and run the demo app you have to choice the target between Mac OSX, iOS device (iPhone or iPad) or iOS simulator, then copy the compiled frameworks from the OSMScout sources for the relevant platform to the right framework subdirectory (see below).

The needed frameworks are:

- OSMScout.framework, the core OSMScout library.
- OSMScoutMap.framework, the generic map drawing library.
- OSMScoutMapIOSX.framework, the platform specialized drawing code for Mac and iOS.

#### Mac OSX
The framework directory used by the Mac target is:
>OSMScoutOSX/OSMScoutOSX/Frameworks

When you have installed the OSMScout sources and the needed dependencies, to compile the library for Mac OSX you might do something like:

```
cd libosmscout
mkdir build.osx
cd build.osx
cmake  -DCMAKE_PREFIX_PATH=/Users/vyskocil/Dev/Qt5.9.1/5.9.1/clang_64/lib/cmake  ..
make
```

#### iOS device (iPhone, iPad)
The framework directory used by the iOS device target is:
>OSMScoutOSX/OSMScoutiOS/Frameworks

When you have installed the OSMScout sources and the needed dependencies, to compile the library for iOS device you might do something like:

```
cd libosmscout
mkdir build.ios
cd build.ios
cmake -DIOS_DEPLOYMENT_TARGET=13.0 -DCMAKE_TOOLCHAIN_FILE=../cmake/iOS.cmake -DMARISA_INCLUDE_DIRS=~/Dev/marisa/lib -DPKG_CONFIG_EXECUTABLE=/usr/local/bin/pkg-config  ..
make
```

#### iOS simulator
The framework directory used by the iOS simulator target is:
>OSMScoutOSX/OSMScoutiOS/Frameworks_simulator

When you have installed the OSMScout sources and the needed dependencies, to compile the library for iOS simulator you might do something like:

```
cd libosmscout
mkdir build.simulator
cd build.simulator
cmake -DIOS_DEPLOYMENT_TARGET=13.0 -DCMAKE_TOOLCHAIN_FILE=../cmake/iOS.cmake -DMARISA_INCLUDE_DIRS=~/Dev/marisa/lib -DPKG_CONFIG_EXECUTABLE=/usr/local/bin/pkg-config -DIOS_PLATFORM=SIMULATOR64 ..
make
```

