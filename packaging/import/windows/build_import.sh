#!/bin/bash

set -e

IMPORT_TOOL_OS=Windows
IMPORT_TOOL_ARCH=x86_64

CXX=g++
CC=gcc

rm -rf build
mkdir build
cd build

cmake -G 'MSYS Makefiles' -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release -DOSMSCOUT_BUILD_MAP_OPENGL=OFF -DOSMSCOUT_BUILD_MAP_AGG=OFF -DOSMSCOUT_BUILD_MAP_CAIRO=OFF -DOSMSCOUT_BUILD_MAP_QT=OFF -DOSMSCOUT_BUILD_MAP_SVG=OFF -DOSMSCOUT_BUILD_MAP_IOSX=OFF -DOSMSCOUT_BUILD_TESTS=OFF -DOSMSCOUT_BUILD_DEMOS=OFF -DOSMSCOUT_BUILD_BINDING_JAVA=OFF -DOSMSCOUT_BUILD_BINDING_CSHARP=OFF -DOSMSCOUT_BUILD_DOC_API=OFF -DOSMSCOUT_BUILD_TOOL_OSMSCOUT2=OFF -DOSMSCOUT_BUILD_TOOL_STYLEEDITOR=OFF -DGPERFTOOLS_USAGE=OFF -DOSMSCOUT_BUILD_TOOL_DUMPDATA=OFF -DOSMSCOUT_BUILD_CLIENT_QT=OFF -DOSMSCOUT_BUILD_GPX=OFF -DOSMSCOUT_BUILD_TOOL_IMPORT=ON -DOSMSCOUT_BUILD_IMPORT=ON -DBUILD_WITH_OPENMP=OFF -DBUILD_IMPORT_TOOL_FOR_DISTRIBUTION=ON ..

make -j2

IMPDIST=libosmscout-importer-$IMPORT_TOOL_OS-$IMPORT_TOOL_ARCH

mkdir -p "$IMPDIST"/bin "$IMPDIST"/stylesheets 

for a in Import/Import libosmscout/libosmscout.dll libosmscout-import/libosmscout_import.dll; do
    cp $a "$IMPDIST"/bin
done

# system libs that stay dynamic
for a in libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll; do
    cp /c/msys64/mingw64/bin/$a "$IMPDIST"/bin
done

cp Import/Import "$IMPDIST"/bin
cp libosmscout/libosmscout.dll "$IMPDIST"/bin

cp ../stylesheets/map.ost "$IMPDIST"/stylesheets
cp ../packaging/import/windows/README.txt "$IMPDIST"
cp ../packaging/import/windows/import.cmd "$IMPDIST"

zip -r $IMPDIST.zip $IMPDIST
