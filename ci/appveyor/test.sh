#!/bin/bash

set -e

BUILDTOOL=${BUILDTOOL:-cmake}

echo "BUILDTOOL: ${BUILDTOOL}"

export LANG=en_US.utf8

if [ "${BUILDTOOL}" = "cmake" ] ; then
    cd build
    ctest
fi
if [ "${BUILDTOOL}" = "meson" ] ; then
    cd debug
    ninja test
fi
