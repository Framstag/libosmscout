#!/bin/bash

set -e

rm -rf build-import
mkdir build-import
cd build-import

cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DOSMSCOUT_BUILD_MAP_OPENGL=OFF -DOSMSCOUT_BUILD_MAP_AGG=OFF -DOSMSCOUT_BUILD_MAP_CAIRO=OFF -DOSMSCOUT_BUILD_MAP_QT=OFF -DOSMSCOUT_BUILD_MAP_SVG=OFF -DOSMSCOUT_BUILD_MAP_IOSX=OFF -DOSMSCOUT_BUILD_TESTS=OFF -DOSMSCOUT_BUILD_DEMOS=OFF -DOSMSCOUT_BUILD_BINDING_JAVA=OFF -DOSMSCOUT_BUILD_BINDING_CSHARP=OFF -DOSMSCOUT_BUILD_DOC_API=OFF -DOSMSCOUT_BUILD_TOOL_OSMSCOUT2=OFF -DOSMSCOUT_BUILD_TOOL_STYLEEDITOR=OFF -DGPERFTOOLS_USAGE=OFF -DOSMSCOUT_BUILD_TOOL_DUMPDATA=OFF -DOSMSCOUT_BUILD_CLIENT_QT=OFF -DOSMSCOUT_BUILD_TOOL_IMPORT=ON -DOSMSCOUT_BUILD_IMPORT=ON -DBUILD_WITH_OPENMP=OFF -DBUILD_IMPORT_TOOL_FOR_DISTRIBUTION=ON ..

make $*

echo "Check which libraries are dynamically linked"
ldd Import/Import

IMPDIST=libosmscout-importer.$IMPORT_TOOL_OS.$IMPORT_TOOL_ARCH

mkdir -p "$IMPDIST"/bin "$IMPDIST"/stylesheets 
cp Import/Import "$IMPDIST"/bin
cp ../packaging/import/linux/README "$IMPDIST"
cp ../packaging/import/linux/import.sh "$IMPDIST"
cp ../stylesheets/map.ost "$IMPDIST"/stylesheets


# EXE="$IMPDIST"/bin/Import

# mkdir -p "$IMPDIST"/lib

# ## lib copy and patch taken from http://unix.stackexchange.com/a/289896
# ## didn't work on gentoo
# # join \
# #     <(ldd "$EXE" | gawk '{if(substr($3,0,1)=="/") print $1,$3}') \
# #     <(patchelf --print-needed "$EXE" ) |cut -d\  -f2 |
# # #copy the lib selection to ./lib
# # xargs -d '\n' -I{} cp --copy-contents {} "$IMPDIST"/lib 

# ldd "$EXE" | gawk '{if(substr($3,0,1)=="/") print $3}' | xargs -d '\n' -I{} cp --copy-contents {} "$IMPDIST"/lib 

# #make the relative lib paths override the system lib path
# patchelf --set-rpath "\$ORIGIN/../lib" "$EXE"

# all is ready for packaging
tar zcvf "$IMPDIST".tar.gz "$IMPDIST"
