#!/bin/bash

set -e

BUILDTOOL=${BUILDTOOL:-cmake}
COMPILER=${COMPILER:-gcc}

echo "BUILDTOOL: ${BUILDTOOL}"
echo "COMPILER:  ${COMPILER}"

export LANG=en_US.utf8

if [ "${COMPILER}" = "clang" ] ; then
    export CC=/usr/bin/clang
    export CXX=/usr/bin/clang++
fi

if [ "${BUILDTOOL}" = "cmake" ] ; then
    CMAKE_ARGS=${CMAKE_ARGS:-"-DCMAKE_BUILD_TYPE=Debug"}
    echo "CMAKE_ARGS: ${CMAKE_ARGS}"

    mkdir -p build
    cd build
    cmake ${CMAKE_ARGS} ..
    make -j `nproc`
fi
if [ "${BUILDTOOL}" = "meson" ] ; then
    meson debug
    cd debug
    ninja
fi
