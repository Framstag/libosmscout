#!/bin/bash

set -e

BUILDTOOL=${BUILDTOOL:-cmake}

echo "BUILDTOOL: ${BUILDTOOL}"

export LANG=en_US.utf8

if [ "${BUILDTOOL}" = "cmake" ] ; then
    (cd build && ctest -j 2 --output-on-failure)
fi
