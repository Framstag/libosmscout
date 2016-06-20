#!/bin/sh

echo "Build start time: `date`"

if [ "$TARGET" = "build" ]; then
  if [ "$BUILDTOOL" = "autoconf" ]; then
    make full
    (cd libosmscout/tests && make check)
  elif [ "$BUILDTOOL" = "cmake" ]; then
    mkdir build
    cd build
    cmake ..
    make
  fi
elif [ "$TARGET" = "website" ]; then
  echo "Building website..."
  cd webpage
  hugo --verbose
fi

echo "Build end time: `date`"

