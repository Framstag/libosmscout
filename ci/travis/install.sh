#!/bin/sh

set -e

echo "Target:     " "$TARGET"
echo "OS:         " "$TRAVIS_OS_NAME"
echo "Build tool: " "$BUILDTOOL"
echo "Compiler:   " "$CXX"

echo "Installation start time: $(date)"

export DEBIAN_FRONTEND=noninteractive

if [ "$TARGET" = "importer" ]; then
  if [ "$TRAVIS_OS_NAME" = "linux" ]; then
    sudo apt-get -qq update

    if [ "$BUILDTOOL" = "cmake" ]; then
      sudo apt-get install -y cmake
    fi

    sudo apt-get install -y \
      pkg-config \
      libxml2-dev \
      libprotobuf-dev protobuf-compiler \
      libmarisa-dev \
      libicu-dev \
      liblzma-dev
  fi
fi

echo "Installation end time: $(date)"
