#!/bin/sh

if [ "$BUILDTOOL" == "autoconf" ]; then
  make full
  (cd libosmscout/tests && make check)
else
  mkdir build
  cd build
  cmake ..
  make
fi

