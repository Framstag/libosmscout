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

    cmake -B build -DCMAKE_UNITY_BUILD=ON -G Ninja "${CMAKE_ARGS}"
    cmake --build build
    cd build && ctest -j 2 --output-on-failure
fi
if [ "${BUILDTOOL}" = "meson" ] ; then
    meson setup --buildtype debugoptimized --unity on debug
    meson compile -C debug
    meosn test -C debug
fi
